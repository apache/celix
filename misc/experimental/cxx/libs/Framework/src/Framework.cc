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

#include <utility>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <set>
#include <vector>
#include <future>
#include <algorithm>
#include <climits>

#include <uuid/uuid.h>

#include <spdlog/spdLog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "BundleController.h"

static auto logger = spdlog::stdout_color_mt("celix::Framework");

extern bool extractBundle(const char *bundleZip, const char *targetPath); //FROM miniunz.c


struct StaticBundleEntry {
    const std::string symbolicName;
    const celix::Properties manifest;
    const std::function<celix::IBundleActivator*(std::shared_ptr<celix::BundleContext>)> activatorFactory;
    const uint8_t *resourcesZip;
    const size_t resourcesZipLen;
};

static struct {
    std::mutex mutex{};
    std::vector<StaticBundleEntry> bundles{};
    std::set<celix::Framework *> frameworks{};
} staticRegistry{};


static void registerFramework(celix::Framework *fw);
static void unregisterFramework(celix::Framework *fw);

namespace {
    //some utils function

    celix::Properties createFwManifest() {
        celix::Properties m{};
        m[celix::MANIFEST_BUNDLE_SYMBOLIC_NAME] = "framework";
        m[celix::MANIFEST_BUNDLE_NAME] = "Framework";
        m[celix::MANIFEST_BUNDLE_GROUP] = "Celix";
        m[celix::MANIFEST_BUNDLE_VERSION] = "3.0.0";
        return m;
    }

    std::string genUUIDString() {
        char uuidStr[37];
        uuid_t uuid;
        uuid_generate(uuid);
        uuid_unparse(uuid, uuidStr);
        return std::string{uuidStr};
    }

    std::string createCwdString() {
        char workdir[PATH_MAX];
        if (getcwd(workdir, sizeof(workdir)) != nullptr) {
            return std::string{workdir};
        } else {
            return std::string{};
        }
    }

    std::string genFwCacheDir(const celix::Properties &/*fwConfig*/) {
        //TODO make configure-able
        return createCwdString() + "/.cache";
    }
}

class celix::Framework::Impl {
public:
    Impl(celix::Framework *_fw, celix::Properties _config) :
            fw{_fw},
            config{std::move(_config)},
            bndManifest{createFwManifest()},
            cwd{createCwdString()},
            fwUUID{genUUIDString()},
            fwCacheDir{genFwCacheDir(config)} {
        logger->debug("Framework {} created", fwUUID);
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    ~Impl() {}

    std::vector<long> listBundles(bool includeFrameworkBundle) const {
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

    long installBundle(
            const std::string& symbolicName,
            std::function<celix::IBundleActivator*(std::shared_ptr<celix::BundleContext>)> actFactory,
            celix::Properties manifest,
            bool autoStart,
            const uint8_t *resourcesZip,
            size_t resourcesZipLen) {
        //TODO on separate thread ?? specific bundle resolve thread ??
        long bndId = -1L;
        if (symbolicName.empty()) {
            //TODO move to cc file and add logger
            //LOG(WARNING) << "Cannot install bundle with a empty symbolic name" << std::endl;
            return bndId;
        }

        std::shared_ptr<celix::BundleController> bndController{nullptr};
        {
            manifest[celix::MANIFEST_BUNDLE_SYMBOLIC_NAME] = symbolicName;
            if (manifest.find(celix::MANIFEST_BUNDLE_NAME) == manifest.end()) {
                manifest[celix::MANIFEST_BUNDLE_NAME] = symbolicName;
            }
            if (manifest.find(celix::MANIFEST_BUNDLE_VERSION) == manifest.end()) {
                manifest[celix::MANIFEST_BUNDLE_NAME] = "0.0.0";
            }
            if (manifest.find(celix::MANIFEST_BUNDLE_GROUP) == manifest.end()) {
                manifest[celix::MANIFEST_BUNDLE_GROUP] = "";
            }

            std::lock_guard<std::mutex> lck{bundles.mutex};
            bndId = bundles.nextBundleId++;
            auto bnd = std::shared_ptr<celix::Bundle>{new celix::Bundle{bndId, this->cacheDir(), this->fw, std::move(manifest)}};
            auto ctx = std::shared_ptr<celix::BundleContext>{new celix::BundleContext{bnd}};
            bndController = std::shared_ptr<celix::BundleController>{new celix::BundleController{std::move(actFactory), bnd, ctx, resourcesZip, resourcesZipLen}};
            bundles.entries.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(bndId),
                                     std::forward_as_tuple(bndController));

            //TODO increase bnd entry usage
        }

        if (bndController) {
            if (autoStart) {
                bool successful = bndController->transitionTo(BundleState::ACTIVE);
                if (!successful) {
                    //TODO move to cc file and add logger
                    //LOG(WARNING) << "Cannot start bundle " << bndController->bundle()->symbolicName() << std::endl;
                }
            }
        }

        return bndId;
    }

    bool startBundle(long bndId) {
        if (bndId == FRAMEWORK_BUNDLE_ID) {
            return startFramework();
        } else {
            return transitionBundleTo(bndId, BundleState::ACTIVE);
        }
    }

    bool stopBundle(long bndId) {
        if (bndId == FRAMEWORK_BUNDLE_ID) {
            return stopFramework();
        } else {
            return transitionBundleTo(bndId, BundleState::INSTALLED);
        }
    }

    bool uninstallBundle(long bndId) {
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

    bool useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const {
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

    int useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFramework) const {
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
        int count = (int)useBundles.size();
        if (includeFramework) {
            count += 1;
        }
        return count;
    }

    std::shared_ptr<celix::ServiceRegistry> registry() const {
        return reg;
    }


    std::string cacheDir() const {
        return fwCacheDir;
    }

    std::string uuid() const {
        return fwUUID;
    }

    std::shared_ptr<celix::BundleContext> frameworkContext() const {
        std::lock_guard<std::mutex> lck(bundles.mutex);
        return bundles.entries.at(celix::FRAMEWORK_BUNDLE_ID)->context();
    }

    bool startFramework() {
        //TODO create cache dir using a process id (and  lock file?).
        //Maybe also move to /var/cache or /tmp and when framework stop delete all framework caches of not running processes

        {
            //Adding framework bundle to the bundles.
            auto bnd = std::shared_ptr<celix::Bundle>{new celix::Bundle{FRAMEWORK_BUNDLE_ID, this->fwCacheDir, this->fw, bndManifest}};
            bnd->setState(BundleState::ACTIVE);
            auto ctx = std::shared_ptr<celix::BundleContext>{new celix::BundleContext{bnd}};
            class EmptyActivator : public IBundleActivator {};
            std::function<celix::IBundleActivator*(std::shared_ptr<celix::BundleContext>)> fac = [](const std::shared_ptr<celix::BundleContext>&) {
                return new EmptyActivator{};
            };
            auto ctr = std::shared_ptr<celix::BundleController>{new celix::BundleController{std::move(fac), std::move(bnd), std::move(ctx), nullptr, 0}};
            std::lock_guard<std::mutex> lck{bundles.mutex};
            bundles.entries[FRAMEWORK_BUNDLE_ID] = std::move(ctr);
        }

        //TODO update state

        std::cout << "Celix Framework Started\n";
        return true;
    }

    bool stopFramework() {
        std::lock_guard<std::mutex> lck{shutdown.mutex};
        if (!shutdown.shutdownStarted) {
            shutdown.future = std::async(std::launch::async, [this]{
                std::vector<long> fwBundles = listBundles(false);
                while (!fwBundles.empty()) {
                    for (auto it = fwBundles.rbegin(); it != fwBundles.rend(); ++it) {
                        stopBundle(*it);
                        uninstallBundle(*it);
                    }
                    fwBundles = listBundles(false);
                }
            });
            shutdown.shutdownStarted = true;
            shutdown.cv.notify_all();
        }

        //TODO update bundle state

        //TODO clean cache dir

        std::cout << "Celix Framework Stopped\n";
        return true;
    }

    bool waitForShutdown() const {
        std::unique_lock<std::mutex> lck{shutdown.mutex};
        shutdown.cv.wait(lck, [this]{return this->shutdown.shutdownStarted;});
        shutdown.future.wait();
        lck.unlock();
        return true;
    }
private:

    celix::Framework * const fw;
    const celix::Properties config;
    const celix::Properties bndManifest;
    const std::string cwd;
    const std::string fwUUID;
    const std::string fwCacheDir;


    struct {
        mutable std::mutex mutex{};
        mutable std::condition_variable cv{};
        std::future<void> future{};
        bool shutdownStarted = false;
    } shutdown{};



    struct {
        std::unordered_map<long, std::shared_ptr<celix::BundleController>> entries{};
        long nextBundleId = FRAMEWORK_BUNDLE_ID + 1;
        mutable std::mutex mutex{};
    } bundles{};


    const std::shared_ptr<celix::ServiceRegistry> reg{new celix::ServiceRegistry{"celix"}};
};

/***********************************************************************************************************************
 * Framework
 **********************************************************************************************************************/

celix::Framework::Framework(celix::Properties config) : pimpl{std::unique_ptr<Impl>{new Impl{this, std::move(config)}}} {
    pimpl->startFramework();
    registerFramework(this);
}

celix::Framework::~Framework() {
    unregisterFramework(this);
    pimpl->stopFramework();
    pimpl->waitForShutdown();
}

celix::Framework::Framework(Framework &&) = default;
celix::Framework& celix::Framework::operator=(Framework&&) = default;


long celix::Framework::installBundle(
        std::string name,
        std::function<celix::IBundleActivator*(std::shared_ptr<celix::BundleContext>)> actFactory,
        celix::Properties manifest,
        bool autoStart,
        const uint8_t *resourcesZip,
        size_t resourcesZipLen) {
    return pimpl->installBundle(std::move(name), std::move(actFactory), std::move(manifest), autoStart, resourcesZip, resourcesZipLen);
}


std::vector<long> celix::Framework::listBundles(bool includeFrameworkBundle) const { return pimpl->listBundles(includeFrameworkBundle); }

bool celix::Framework::useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const {
    return pimpl->useBundle(bndId, std::move(use));
}

int celix::Framework::useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFrameworkBundle) const {
    return pimpl->useBundles(std::move(use), includeFrameworkBundle);
}

bool celix::Framework::startBundle(long bndId) { return pimpl->stopBundle(bndId); }
bool celix::Framework::stopBundle(long bndId) { return pimpl->stopBundle(bndId); }
bool celix::Framework::uninstallBundle(long bndId) { return pimpl->uninstallBundle(bndId); }
std::shared_ptr<celix::ServiceRegistry> celix::Framework::registry() const { return pimpl->registry(); }

bool celix::Framework::waitForShutdown() const { return pimpl->waitForShutdown(); }
std::string celix::Framework::cacheDir() const {
    return pimpl->cacheDir();
}

std::string celix::Framework::uuid() const {
    return pimpl->uuid();
}

std::shared_ptr<celix::BundleContext> celix::Framework::context() const {
    return pimpl->frameworkContext();
}

/***********************************************************************************************************************
 * Celix 'global' functions
 **********************************************************************************************************************/

void celix::registerStaticBundle(
        std::string symbolicName,
        std::function<celix::IBundleActivator*(const std::shared_ptr<celix::BundleContext>&)> bundleActivatorFactory,
        celix::Properties manifest,
        const uint8_t *resourcesZip,
        size_t resourcesZipLen) {
    std::lock_guard<std::mutex> lck{staticRegistry.mutex};
    for (auto fw : staticRegistry.frameworks) {
        fw->installBundle(symbolicName, bundleActivatorFactory, manifest, true, resourcesZip, resourcesZipLen);
    }
    staticRegistry.bundles.emplace_back(StaticBundleEntry{
        .symbolicName = std::move(symbolicName),
        .manifest = std::move(manifest),
        .activatorFactory = std::move(bundleActivatorFactory),
        .resourcesZip = resourcesZip,
        .resourcesZipLen = resourcesZipLen});
}

static void registerFramework(celix::Framework *fw) {
    std::lock_guard<std::mutex> lck{staticRegistry.mutex};
    staticRegistry.frameworks.insert(fw);
    for (auto &entry : staticRegistry.bundles) {
        fw->installBundle(entry.symbolicName, entry.activatorFactory, entry.manifest, true, entry.resourcesZip, entry.resourcesZipLen);
    }
}

static void unregisterFramework(celix::Framework *fw) {
    std::lock_guard<std::mutex> lck{staticRegistry.mutex};
    staticRegistry.frameworks.erase(fw);
}