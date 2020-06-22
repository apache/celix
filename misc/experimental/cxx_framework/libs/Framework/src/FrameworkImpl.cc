/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include "celix/Framework.h"

#include <memory>
#include <utility>
#include <map>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <future>
#include <algorithm>
#include <climits>

#include <uuid/uuid.h>

#include "BundleContextImpl.h"
#include "BundleController.h"

#define LOGGER celix::getLogger("celix::Framework")

extern bool extractBundle(const char *bundleZip, const char *targetPath); //FROM miniunz.c


struct StaticBundleEntry {
    const std::string symbolicName;
    const celix::Properties manifest;
    const std::function<celix::IBundleActivator *(std::shared_ptr<celix::BundleContext>)> activatorFactory;
    const uint8_t *resourcesZip;
    const size_t resourcesZipLen;
};

static struct {
    std::mutex mutex{};
    std::vector<StaticBundleEntry> bundles{};
    std::unordered_set<celix::Framework*> frameworks{};
} staticRegistry{};


static void registerFramework(celix::Framework* fw);

static void unregisterFramework(celix::Framework *fw);

namespace celix::impl {
    //some utils functions
    static celix::Properties createFwManifest() {
        celix::Properties m{};
        m[celix::MANIFEST_BUNDLE_SYMBOLIC_NAME] = "celix::Framework";
        m[celix::MANIFEST_BUNDLE_NAME] = "Celix Framework";
        m[celix::MANIFEST_BUNDLE_GROUP] = "celix";
        m[celix::MANIFEST_BUNDLE_VERSION] = "3.0.0";
        return m;
    }

    static std::string genUUIDString() {
        char uuidStr[37];
        uuid_t uuid;
        uuid_generate(uuid);
        uuid_unparse(uuid, uuidStr);
        return std::string{uuidStr};
    }

    static std::string createCwdString() {
        char workdir[PATH_MAX];
        if (getcwd(workdir, sizeof(workdir)) != nullptr) {
            return std::string{workdir};
        } else {
            return std::string{};
        }
    }

    static std::string genFwCacheDir(const celix::Properties &/*fwConfig*/) {
        //TODO make configure-able
        return createCwdString() + "/.cache";
    }

    class FrameworkImpl : public celix::Framework {
    public:
        FrameworkImpl(celix::Properties _config) :
                config{std::move(_config)},
                bndManifest{createFwManifest()},
                cwd{createCwdString()},
                fwUUID{genUUIDString()},
                fwCacheDir{genFwCacheDir(config)} {
            LOGGER->trace("Celix Framework {} created", shortUUID());
        }

        ~FrameworkImpl() override {
            LOGGER->trace("Celix Framework {} destroyed", shortUUID());
        }

        std::vector<long> listBundles(bool includeFrameworkBundle) const override {
            std::vector<long> result{};
            std::lock_guard<std::mutex> lock{bundles.mutex};
            for (auto &entry : bundles.entries) {
                if (entry.second->bundle()->id() == FRAMEWORK_BUNDLE_ID) {
                    if (includeFrameworkBundle) {
                        result.push_back(entry.first);
                    }
                } else {
                    result.push_back(entry.first);
                }
            }
            std::sort(result.begin(), result.end());//ensure that the bundles are order by bndId -> i.e. time of install
            return result;
        }

        virtual long installBundle(
                std::string symbolicName,
                std::function<celix::IBundleActivator *(std::shared_ptr<celix::BundleContext>)> actFactory,
                celix::Properties manifest,
                bool autoStart,
                const uint8_t *resourcesZip,
                size_t resourcesZipLen) override {
            //TODO on separate thread ?? specific bundle resolve thread ??
            long bndId = -1L;
            if (symbolicName.empty()) {
                LOGGER->error("Cannot install bundle with an empty symbolic name!");
                return bndId;
            }

            std::shared_ptr<celix::BundleController> bndController{nullptr};
            {
                manifest[celix::MANIFEST_BUNDLE_SYMBOLIC_NAME] = symbolicName;
                if (manifest.find(celix::MANIFEST_BUNDLE_NAME) == manifest.end()) {
                    manifest[celix::MANIFEST_BUNDLE_NAME] = symbolicName;
                }
                if (manifest.find(celix::MANIFEST_BUNDLE_VERSION) == manifest.end()) {
                    manifest[celix::MANIFEST_BUNDLE_VERSION] = "0.0.0";
                }
                if (manifest.find(celix::MANIFEST_BUNDLE_GROUP) == manifest.end()) {
                    manifest[celix::MANIFEST_BUNDLE_GROUP] = "";
                }

                std::lock_guard<std::mutex> lck{bundles.mutex};
                bndId = bundles.nextBundleId++;
                auto bnd = std::make_shared<celix::Bundle>(bndId, this->cacheDir(), this /*TODO improve*/,
                                                           std::move(manifest));
                auto ctx = std::make_shared<celix::impl::BundleContextImpl>(bnd);
                bndController = std::make_shared<celix::BundleController>(std::move(actFactory), bnd, ctx, resourcesZip,
                                                                          resourcesZipLen);
                bundles.entries.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(bndId),
                                        std::forward_as_tuple(bndController));

                //TODO increase bnd entry usage
            }

            if (bndController) {
                if (autoStart) {
                    bool successful = bndController->transitionTo(BundleState::ACTIVE);
                    if (!successful) {
                        //TODO move to cc file and add LOGGER
                        //LOG(WARNING) << "Cannot start bundle " << bndController->bundle()->symbolicName() << std::endl;
                    }
                }
            }

            return bndId;
        }

        bool startBundle(long bndId) override {
            if (bndId == FRAMEWORK_BUNDLE_ID) {
                return startFramework();
            } else {
                return transitionBundleTo(bndId, BundleState::ACTIVE);
            }
        }

        bool stopBundle(long bndId) override {
            if (bndId == FRAMEWORK_BUNDLE_ID) {
                return stopFramework();
            } else {
                return transitionBundleTo(bndId, BundleState::INSTALLED);
            }
        }

        bool uninstallBundle(long bndId) override {
            bool uninstalled = false;
            std::shared_ptr<celix::BundleController> removed{nullptr};
            {
                std::lock_guard<std::mutex> lck{bundles.mutex};
                auto it = bundles.entries.find(bndId);
                if (it != bundles.entries.end()) {
                    removed = std::move(it->second);
                    bundles.entries.erase(it);

                }
            }
            if (removed) {
                bool stopped = removed->transitionTo(BundleState::INSTALLED);
                if (stopped) {
                    //TODO check and wait till bundle is not used anymore. is this needed (shared_ptr) or just let access
                    //to filesystem fail ...
                } else {
                    //add bundle again -> not uninstalled
                    std::lock_guard<std::mutex> lck{bundles.mutex};
                    bundles.entries[bndId] = std::move(removed);
                }
            }
            return uninstalled;
        }

        bool transitionBundleTo(long bndId, BundleState desired) {
            bool successful = false;
            std::shared_ptr<celix::BundleController> match{nullptr};
            {
                std::lock_guard<std::mutex> lck{bundles.mutex};
                auto it = bundles.entries.find(bndId);
                if (it != bundles.entries.end()) {
                    match = it->second;
                }
            }
            if (match) {
                successful = match->transitionTo(desired);
            }
            return successful;
        }

        bool useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const override {
            bool called = false;
            std::shared_ptr<celix::BundleController> match = nullptr;
            {
                std::lock_guard<std::mutex> lck{bundles.mutex};
                auto it = bundles.entries.find(bndId);
                if (it != bundles.entries.end()) {
                    match = it->second;
                    //TODO increase usage
                }
            }
            if (match) {
                use(*match->bundle());
                called = true;
                //TODO decrease usage -> use shared ptr instead
            }
            return called;
        }

        int useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFramework) const override {
            std::map<long, std::shared_ptr<celix::BundleController>> useBundles{};
            {
                std::lock_guard<std::mutex> lck{bundles.mutex};
                for (const auto &it : bundles.entries) {
                    if (it.second->bundle()->id() == FRAMEWORK_BUNDLE_ID) {
                        if (includeFramework) {
                            useBundles[it.first] = it.second;
                        }
                    } else {
                        useBundles[it.first] = it.second;
                    }
                }
            }

            for (const auto &cntr : useBundles) {
                use(*cntr.second->bundle());
            }
            int count = (int) useBundles.size();
            if (includeFramework) {
                count += 1;
            }
            return count;
        }

        std::shared_ptr<celix::ServiceRegistry> registry() const override {
            return reg;
        }


        std::string cacheDir() const override {
            return fwCacheDir;
        }

        std::string uuid() const override {
            return fwUUID;
        }

        std::string shortUUID() const override {
            return fwUUID.substr(0, 4);
        }

        std::shared_ptr<celix::BundleContext> context() const override {
            std::lock_guard<std::mutex> lck(bundles.mutex);
            return bundles.entries.at(celix::FRAMEWORK_BUNDLE_ID)->context();
        }


        bool waitForShutdown() const override {
            std::unique_lock<std::mutex> lck{shutdown.mutex};
            shutdown.cv.wait(lck, [this] { return this->shutdown.shutdownDone; });
            return true;
        }

        bool startFramework() {
            //TODO create cache dir using a process id (and  lock file?).
            //Maybe also move to /var/cache or /tmp and when framework stop delete all framework caches of not running processes

            {
                //Adding framework bundle to the bundles.
                auto bnd = std::make_shared<celix::Bundle>(
                        FRAMEWORK_BUNDLE_ID, this->fwCacheDir, this /*TODO improve*/, bndManifest);
                bnd->setState(BundleState::ACTIVE);
                auto ctx = std::make_shared<celix::impl::BundleContextImpl>(bnd);
                class EmptyActivator : public IBundleActivator {
                };
                std::function<celix::IBundleActivator *(std::shared_ptr<celix::BundleContext>)> fac = [](
                        const std::shared_ptr<celix::BundleContext> &) {
                    return new EmptyActivator{};
                };
                auto ctr = std::make_shared<celix::BundleController>(
                        std::move(fac), std::move(bnd), std::move(ctx), nullptr, 0);
                std::lock_guard<std::mutex> lck{bundles.mutex};
                bundles.entries[FRAMEWORK_BUNDLE_ID] = std::move(ctr);
            }

            //TODO update state

            LOGGER->info("Celix Framework {} Started.", shortUUID());
            return true;
        }

        bool stopFramework() {
            std::vector<long> fwBundles = listBundles(false);
            while (!fwBundles.empty()) {
                for (auto it = fwBundles.rbegin(); it != fwBundles.rend(); ++it) {
                    stopBundle(*it);
                    uninstallBundle(*it);
                }
                fwBundles = listBundles(false);
            }

            {
                std::lock_guard<std::mutex> lck{shutdown.mutex};
                shutdown.shutdownDone = true;
                shutdown.cv.notify_all();
            }


            //TODO clean cache dir

            LOGGER->info("Celix Framework {} Stopped.", shortUUID());
            return true;
        }
    private:

        const celix::Properties config;
        const celix::Properties bndManifest;
        const std::string cwd;
        const std::string fwUUID;
        const std::string fwCacheDir;

        struct {
            mutable std::mutex mutex{};
            mutable std::condition_variable cv{};
            bool shutdownDone = false;
        } shutdown{};


        struct {
            std::map<long, std::shared_ptr<celix::BundleController>> entries{};
            long nextBundleId = FRAMEWORK_BUNDLE_ID + 1;
            mutable std::mutex mutex{};
        } bundles{};


        const std::shared_ptr<celix::ServiceRegistry> reg{celix::ServiceRegistry::create("celix")};
    };
}

std::shared_ptr<celix::Framework> celix::Framework::create(celix::Properties config) {
    auto fw = std::shared_ptr<celix::impl::FrameworkImpl>{new celix::impl::FrameworkImpl{std::move(config)}, [](auto *fwPtr) {
        fwPtr->stopFramework();
        unregisterFramework(fwPtr);
        delete fwPtr;
    }};
    registerFramework(fw.get());
    fw->startFramework();
    return fw;
}

void celix::registerStaticBundle(
        std::string symbolicName,
        std::function<celix::IBundleActivator *(const std::shared_ptr<celix::BundleContext> &)> bundleActivatorFactory,
        celix::Properties manifest,
        const uint8_t *resourcesZip,
        size_t resourcesZipLen) {
    std::lock_guard<std::mutex> lck{staticRegistry.mutex};
    for (auto& fw : staticRegistry.frameworks) {
        fw->installBundle(symbolicName, bundleActivatorFactory, manifest, true, resourcesZip, resourcesZipLen);
    }
    staticRegistry.bundles.emplace_back(StaticBundleEntry{
            .symbolicName = std::move(symbolicName),
            .manifest = std::move(manifest),
            .activatorFactory = std::move(bundleActivatorFactory),
            .resourcesZip = resourcesZip,
            .resourcesZipLen = resourcesZipLen});
}

static void registerFramework(celix::Framework* fw) {
    std::lock_guard<std::mutex> lck{staticRegistry.mutex};
    staticRegistry.frameworks.insert(fw);
    for (auto &entry : staticRegistry.bundles) {
        fw->installBundle(entry.symbolicName, entry.activatorFactory, entry.manifest, true, entry.resourcesZip,
                          entry.resourcesZipLen);
    }
}

static void unregisterFramework(celix::Framework *fw) {
    std::lock_guard<std::mutex> lck{staticRegistry.mutex};
    staticRegistry.frameworks.erase(fw);
}