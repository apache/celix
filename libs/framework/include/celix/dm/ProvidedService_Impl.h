/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

inline celix::dm::BaseProvidedService::BaseProvidedService(celix_dm_component_t* _cmp, std::string _svcName, void* _svc, bool _cppService) : cCmp{_cmp}, svcName{std::move(_svcName)}, svcPointer{_svc}, cppService{_cppService} {}

inline const std::string &BaseProvidedService::getName() const {
    return svcName;
}

inline const std::string &BaseProvidedService::getVersion() const {
    return svcVersion;
}

inline bool BaseProvidedService::isCppService() const {
    return cppService;
}

inline void *BaseProvidedService::getService() const {
    return svcPointer;
}

inline const celix::dm::Properties &BaseProvidedService::getProperties() const {
    return properties;
}

inline void BaseProvidedService::runBuild() {
    if (!provideAddedToCmp) {
        //setup c properties
        celix_properties_t *cProperties = properties_create();
        properties_set(cProperties, CELIX_FRAMEWORK_SERVICE_LANGUAGE,
                       cppService ? CELIX_FRAMEWORK_SERVICE_CXX_LANGUAGE : CELIX_FRAMEWORK_SERVICE_C_LANGUAGE);
        for (const auto &pair : properties) {
            properties_set(cProperties, pair.first.c_str(), pair.second.c_str());
        }

        const char *cVersion = svcVersion.empty() ? nullptr : svcVersion.c_str();
        celix_dmComponent_addInterface(cCmp, svcName.c_str(), cVersion, svcPointer, cProperties);
    }
    provideAddedToCmp = true;
}

template<typename T, typename I>
ProvidedService<T,I>& celix::dm::ProvidedService<T,I>::setVersion(std::string v) {
    svcVersion = std::move(v);
    return *this;
}

template<typename T, typename I>
ProvidedService<T, I> &ProvidedService<T, I>::setProperties(celix::dm::Properties p) {
    properties = std::move(p);
    return *this;
}

template<typename T, typename I>
template<typename U>
ProvidedService<T, I> &ProvidedService<T, I>::addProperty(const std::string &key, const U &value) {
    properties[key] = std::to_string(value);
    return *this;
}

template<typename T, typename I>
ProvidedService<T, I> &ProvidedService<T, I>::build() {
    this->runBuild();
    return *this;
}

template<typename T, typename I>
ProvidedService<T, I>::ProvidedService(celix_dm_component_t *_cmp, std::string svcName, I* _svc, bool _cppService)
        :BaseProvidedService(_cmp, svcName, static_cast<void*>(_svc), _cppService) {

}

template<typename T, typename I>
ProvidedService<T, I> &ProvidedService<T, I>::addProperty(const std::string &key, const char *value) {
    properties[key] = std::string{value};
    return *this;
}

template<typename T, typename I>
ProvidedService<T, I> &ProvidedService<T, I>::addProperty(const std::string &key, const std::string &value) {
    properties[key] = value;
    return *this;
}
