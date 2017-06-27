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
#ifndef CELIX_H_
#define CELIX_H_

/** General remarks
 *
 * - Id instead of pointers
 * In many cases a object id as uint64_t is used instead of pointer. This to prevent storing pointers to shared
 * resources with a volatile lifecycle (i.e. almost everything in a dynamic module/service framework)
 *
 * - Resilient API
 * All celix functions should silently allow to be called by NULL arguments and invalid ids.
 * For example the celix_bundle_name(NULL) call will silently return NULL.
 *
 * - Naming
 * The API mostly moved away from the typical Java getter / setter approach: Nouns are used
 * for retrieving values which does not have a side effect (also the first argument is a const).
 * THe exception for this are function which can be used as questions in logical operation, these are prefix
 * with a is or has (e.g. hasKey / isValid). Verbs are used for functions with side effects.
 * For callbacks which act on a certain event a on prefix is used (e.g. onAddingService).
 *
 * - Multi language support
 * The aim is to add support for multiple language with the C being the common denominator, foreseen languages:
 * C, C++, Rust, Swift & Python.
 * The API is prepared for multi language support.
 */

//TODO log marcros (FW, MODULE & BUNDLE)

#include <stddef.h> //NULL
#include <stdint.h> //uint64_t
#include <stdio.h> //FILE
#include <pthread.h> //pthread_mutex_t

#ifdef __cplusplus
extern "C" {
#endif

//celix.h
#include "celix/types.h"
#include "celix/utils/properies.h"
#include "celix/utils/map.h"
#include "celix/framework.h"
#include "celix/bundle.h"
#include "celix/module.h"
#include "celix/framework_services.h"

#define CELIX_API_VERSION 3

#define CELIX_LANG_C "C"
#define CELIX_LANG_CXX "C++"
#define CELIX_LANG_SWIFT "Swift"
#define CELIX_LANG_PYTHON "Python"




//celix/utils/map.h

/**
 * The celix map can store values under a string keys.
 * The values can have different types.
 *
 * The map API is not thread safe.
 */

#include <stdbool.h> //bool

typedef struct celix_map celix_map;

typedef enum celix_map_value_type {
    CELIX_MAP_TYPE_UNKNOWN      = 0,
    CELIX_MAP_TYPE_STRING       = 1,
    CELIX_MAP_TYPE_PTR          = 2,
    CELIX_MAP_TYPE_CONST_PTR    = 3,
    CELIX_MAP_TYPE_INT8         = 4,
    CELIX_MAP_TYPE_UINT8        = 5,
    CELIX_MAP_TYPE_INT16        = 6,
    CELIX_MAP_TYPE_UINT16       = 7,
    CELIX_MAP_TYPE_INT32        = 8,
    CELIX_MAP_TYPE_UINT32       = 9,
    CELIX_MAP_TYPE_INT64        = 10,
    CELIX_MAP_TYPE_UINT64       = 11,
    CELIX_MAP_TYPE_FLOAT        = 12,
    CELIX_MAP_TYPE_DOUBLE       = 13,
} celix_map_value_type;

typedef struct celix_map_entry {
    const char* key;
    celix_map_entry_type type;
    union {
        const char* strVal;
        void* ptrVal;
        void* constPtrVal;
        int8_t int8Val;
        int16_t int16Val;
        int32_t int32Val;
        int64_t int64Val;
        uint8_t uint8Val;
        uint16_t uint8Val;
        uint32_t uint8Val;
        uint64_t uint8Val;
        float floatVal;
        double doubleVal;
    } value;
} celix_map_entry;

/**
 * Creates a new map (hashmap or maybe BST (sys/tree.h)?)
 */
celix_map* celix_map_create(void);

/**
 * Copies (deep copy) a map
 */
celix_map* celix_map_copy(const celix_map* map);

/**
 * Destroys the map and release the memory.
 * Specifically this means that the map entries and keys are freed.
 */
void celix_map_destroy(celix_map* map);

/**
 * Returns an array of keys. The map is still owner of this array.
 * @param map The celix map
 * @param len If not NULL will be set to the size of the map.
 * @return If len is NULL this returns NULL, else a array of the keys.
 */
const char* celix_map_keys(const celix_map* map, size_t* len);

/**
 * Returns an array of entries. The map is still owner of this array.
 * @param map The celix map
 * @param len If not NULL will be set to the size of the map.
 * @return If len is NULL this returns NULL, else a array of the entries.
 */
const celix_map_entry* celix_map_entries(const celix_map* map, size_t* len);

/**
 * Returns the nr of entries of the map
 */
size_t celix_map_size(const celix_map* map);

/**
 * Returns whether the map has an entry with the provided key
 */
bool celix_map_hasKey(const celix_map* map, const char* key);

/**
 * Returns the value type of the entry for the provided key
 * or CELIX_MAP_VALUE_TYPE_UNKNOWN if the is no entry for the
 * provided key.
 */
celix_map_value_type celix_map_valueType(const celix_map* map, const char* key);

/**
 * Returns the string value for the provided key.
 * If there is no entry for the provided key or if the entry is not a string
 * type the provided defaultValue will be returned.
 */
const char* celix_map_stringValue(const celix_map* map, const char* key, const char* defaultValue);

/**
 * Adds or replaces an entry fot he provided key. The entry will be of a ptr type.
 * The key and value string will be copied into the entry if its a new entry.
 * If this replaces an entry, only the value will copied to the entry and previous
 * value will be freed.
 */
void celix_map_putString(celix_map* map, const char* key, const char* value);

/**
 * Returns the ptr value for the provided key.
 * If there is no entry for the provided key or if the entry is not a ptr
 * type the provided defaultValue will be returned.
 */
void* celix_map_ptrValue(const celix_map* map, const char* key, const void* defaultValue);

/**
 * Adds or replaces an entry fot he provided key. The entry will be of a ptr type.
 * The key string will be copied into the entry if its a new entry.
 * If this replaces an entry, only the value will be changed.
 */
void celix_map_putPtr(celix_map* map, const char* key, void* value);

/**
 * Returns the const ptr value for the provided key.
 * If there is no entry for the provided key or if the entry is not a ptr
 * type the provided defaultValue will be returned.
 */
const void* celix_map_constPtrValue(const celix_map* map, const char* key, const void* defaultValue);

/**
 * Adds or replaces an entry fot he provided key. The entry will be of a const ptr type.
 * The key string will be copied into the entry if its a new entry.
 * If this replaces an entry, only the value will be changed.
 */
void celix_map_putConstPtr(celix_map* map, const char* key, const void* value);


int8_t celix_map_int8Value(const celix_map* map, const char* key, int8_t defaultValue);
void celix_map_putInt8(celix_map* map, const char* key, int8_t value);

uint8_t celix_map_uint8Value(const celix_map* map, const char* key, uin8_t defaultValue);
void celix_map_putUint8(celix_map* map, const char* key, uint8_t value);

int16_t celix_map_int16Value(const celix_map* map, const char* key, int16_t defaultValue);
void celix_map_putInt16(celix_map* map, const char* key, int16_t value);

uint16_t celix_map_uint16Value(const celix_map* map, const char* key, uin16_t defaultValue);
void celix_map_putUint16(celix_map* map, const char* key, uint16_t value);

int32_t celix_map_int32Value(const celix_map* map, const char* key, int32_t defaultValue);
void celix_map_putInt32(celix_map* map, const char* key, int32_t value);

uint32_t celix_map_uint32Value(const celix_map* map, const char* key, uin32_t defaultValue);
void celix_map_putUint32(celix_map* map, const char* key, uint32_t value);

int64_t celix_map_int64Value(const celix_map* map, const char* key, int64_t defaultValue);
void celix_map_putInt64(celix_map* map, const char* key, int64_t value);

uint64_t celix_map_uint64Value(const celix_map* map, const char* key, uin64_t defaultValue);
void celix_map_putUint64(celix_map* map, const char* key, uint64_t value);

float celix_map_floatValue(const celix_map* map, const char* key, float defaultValue);
void celix_map_putFloat(celix_map* map, const char* key, float value);

double celix_map_doubleValue(const celix_map* map, const char* key, double defaultValue);
void celix_map_putDouble(celix_map* map, const char* key, double value);

//celix/utils/properties.h

/**
 * The celix properties can store string values under a string keys.
 * Properties can be created from properties files (see https://en.wikipedia.org/wiki/.properties)
 *
 * The properties API is not thread safe.
 */

#include <stdbool.h> //bool
#include <stdio.h> //FILE

typedef struct celix_properties celix_properties;

typedef struct celix_properties_entry {
    const char* key;
    const char* value;
} celix_properties_entry;

/**
 * Creates a properties object
 */
celix_properties* celix_properties_create(void);

/**
 * Creates a properties object from the provided properties file.
 * Returns NULL if the properties file is invalid.
 */
celix_properties* celix_properties_createFromFile(FILE* file);

/**
 * Creates a properties object from the provided properties formatted string
 * Returns NULL if the content is invalid.
 */
celix_properties* celix_properties_createFromContent(const char* content);

/**
 * Copies (deep copy) a properties object
 */
celix_properties* celix_properties_copy(const celix_properties* properties);

/**
 * Destroy a properties object. Also frees the keys and values
 */
void celix_properties_destroy(celix_properties* props);

/**
 * Returns an array of keys. The properties is still owner of this array.
 * @param map The celix properties
 * @param len If not NULL will be set to the size of the map.
 * @return If len is NULL this returns NULL, else a array of the keys.
 */
const char* celix_properties_keys(const celix_properties* props, size_t* len);

/**
 * Returns an array of entries. The properties is still owner of this array.
 * @param map The celix properties
 * @param len If not NULL will be set to the size of the map.
 * @return If len is NULL this returns NULL, else a array of the entries.
 */
const celix_properties_entry* celix_properties_entries(const celix_properties* props, size_t* len);

/**
 * Returns the nr of properties of the properties object.
 */
size_t celix_properties_size(const celix_properties* props);

/**
 * Returns whether the properties has an entry with the provided key
 */
bool celix_properties_hasKey(const celix_properties* props, const char* key);

/**
 * Returns the property value for the provided key.
 * Returns the defaultValue is there is no entry for the provided key.
 */
const char* celix_properties_property(const celix_properties* props, const char* key, const char* defaultValue);

/**
 * Set the property for the provided key/value. Creating a copy of the key and value.
 * If there was already a property with the same key, the previous value will be replaced and freed.
 */
void celix_properties_setProperty(celix_properties* props, const char* key, const char* value);



//celix/types.h

//Opaque types. struct are defined in source files or non public header files
typedef struct celix_framework celix_framework;
typedef struct celix_bundle celix_bundle;
typedef struct celix_module celix_module;
typedef struct celix_service_tracker celix_service_tracker;
typedef struct celix_dependency_manager celix_dependency_manager;
typedef struct celix_component celix_component;
typedef struct celix_service_dependency celix_service_dependency;

//TODO, discuss create typedef per id type?
//typedef uint64_t celix_framework_id;
//typedef uint64_t celix_bundle_id;
//typedef uint64_t celix_module_id;
//typedef uint64_t celix_service_id;
//typedef uint64_t celix_service_factor_id;
//typedef uint64_t celix_service_tracker_id;
//typedef uint64_t celix_service_tracker_listener_id;
//typedef uint64_t celix_component_id;

//service factory function pointers
typedef struct celix_service_factory {
    void* handle;
    int (*getService)(void* handle, const celix_module* mod, void** svc /*out*/);
    int (*ungetService)(void* handle, const celix_module* mod, void* svc);
} celix_service_factory;

//Opaque type which should be defined by users
typedef struct celix_module_context celix_module_context;

//celix/framework.h
/**
 * The Framework API can be used to control the lifecycle the framework,
 * retrieve started frameworks.
 */


/**
 * Creates a new Celix framework.
 * Note that this function will block if an framework is created, started, stopped or destroyed.
 *
 * The framework will take ownership of the provided properties.
 *
 * @param config The framework configuration. Will also be provided to the modules
 * @return a ptr to the new framework is successful or NULL.
 */
celix_framework* celix_framework_create(celix_properties* config, celix_framwork_t** out);

/**
 * Starts the provided Celix framework
 * Note that this function will block if an framework is created, started, stopped or destroyed.
 * @return 0 if successful
 */
int celix_framework_start(celix_framework* fw);

/**
 * Stops the provided Celix framework
 * Note that this function will block if an other framework is created, started, stopped or destroyed.
 * @return 0 if successful
 */
int celix_framework_stop(celix_framework* fw);

/**
 * Destroys the provided Celix framework
 * Note that this function will block if an other framework is created, started, stopped or destroyed.
 * @return 0 if successful
 */
int celix_framework_destroy(celix_framework* fw);

/**
 * Blocks until the provided framework is stopped.
 * @return 0 if successful, 1 if interrupted, < 0 if error
 */
int celix_framework_waitForStop(celix_framework* fw);

/**
 * Returns the id of the framework. This id is unique within a single process
 */
uint64_t celix_framework_id(const celix_framework* fw);

/**
 * Returns the uuid of the framework. This uuid is global unique.
 */
const char* celix_framework_uuid(const celix_framework* fw);

/**
 * Returns the framework modules for the provided framework (e.g. module 1, name framework)
 */
celix_module* celix_framework_module(const framework* fw);

/**
 * Returns the framework ids for this process.
 * @param arr The array to store the ids
 * @param arrSize The available size of the provided arr
 * @return the actual number ids added to the array
 */
size_t celix_frameworkIds(void, uint64_t* arr, size_t arrSize);

/**
 * Returns the framework for the provided id or NULL if the framework id is invalid.
 */
celix_framework* celix_framework(uint64_t frameworkId);



//celix/bundle.h

/**
 * Returns the framework which owns the bundle
 */
celix_framework* celix_bundle_framework(const celix_bundle* bundle);

/**
 * Returns the id of te bundle
 */
uint64_t celix_bundle_id(const celix_bundle* bundle);

/**
 * Returns the module of the bundle.
 */
celix_module* celix_bundle_module(const celix_bundle* bundle);



//celix/module.h

/**
 * The module can be used to register/unregister services, create service trackers,
 * add service tracker listeners, register/unregister service factories, get module resources,
 * store and get files from the module store  and retrieve the module's dependency manager.
 *
 * The module API is thread safe
 */

typedef enum celix_module_state {
    CELIX_MODULE_STATE_UNKNOWN      = 0,
    CELIX_MODULE_STATE_INSTALLED    = 1,
    CELIX_MODULE_STATE_RESOLVED     = 2,
    CELIX_MODULE_STATE_STARTING     = 3,
    CELIX_MODULE_STATE_ACTIVE       = 4,
    CELIX_MODULE_STATE_STOPPING     = 5
} celix_module_state;

/**
 * Returns the module id
 */
uint64_t celix_module_id(const celix_module* mod);

/**
 * Returns the celix framwork which owns the module
 */
celix_framework* celix_module_framework(const celix_module* mod);

/**
 * Returns the state of the module.
 */
celix_module_state celix_module_state(const celix_module* mod);


/**
 * Returns the Manifest info in the form of properties (key,value)
 */
const celix_properties* celix_module_manifest(const celix_module* mod);

/**
 * Returns the bundle for this module.
 * @param mod The module
 * @return The bundle or NULL if the module has no bundle (i.e. a library module)
 */
celix_bundle* celix_module_bundle(const celix_module* mod);

/**
 * Set the module context. The module context is a user defined struct that can be used to store user specific data.
 * The module only stores the pointer. A module implementer is still responsible to free the module_context.
 *
 * @param mod The module
 * @param ctx The module context
 */
void celix_module_setContext(celix_module* mod, celix_module_context* ctx);

/**
 * Returns the module context or NULL if the module context is not set.
 */
celix_module_context* celix_module_getContext(const celix_module* mod);

/**
 * Returns the module name.
 */
const char* celix_module_name(const celix_module* mod);

/**
 * Returns the module version. in for format of
 * <major>.<minor>.<micro>-<qualifier>
 * @param mod The module
 * @param major If not NULL, will be set the the major version part.
 * @param minor If not NULL, will be set the the minor version part.
 * @param micro If not NULL, will be set the the micro version part.
 * @param qualifier If not NULL, will be set the the qualifier version part.
 * @return The complete version string
 */
const char* celix_module_version(const celix_module* mod, const char** major /*out*/, const char** minor /*out*/, const char** micro /*out*/, const char** qualifier /*out*/);

/**
 * Returns the property using the framework properties and env variables.
 *
 * If a property is present in the framework properties (config.properties) this will be returned,
 * else a the framework will try to lookup an environment variable for the provided key.
 * If there is no property nor environment variable for the provided key, the defaultValue will be returned.
 *
 * @param mod The module
 * @param name The property name
 * @param defaultValue The default value to use if the property is not set
 * @return The property value or the defaultValue if the property is not set.
 */
const char* celix_module_property(const celix_module* mod, const char* name, const char* defaultValue); //get properties from config.properties and falling back to environment variables

/**
 * Register service for the provide module for the C language.
 * If properties are provided the framework will take ownership.
 *
 * @param mod The module owning the service
 * @param serviceName The service name
 * @param svcVersion The service version. NULL will log a warning
 * @param svc The service
 * @param properties Optional properties containing the meta data of the service
 * @return 0 if unsuccessful else the service id
 */
uint64_t celix_module_registerService(celix_module* mod, const char* serviceName, const char* svcVersion, void* svc, celix_properties* properties);

/**
 * 'type safe' version of the celix_module_registerservice function.
 * Which takes as second argument the service type.
 */
#define CELIX_MODULE_REGISTER_SERVICE(mod, svcType, svcVersion, svc, properties) \
    do { \
        (svcType) tmptype = (svc); \
        const char* svcname = ##svcType; \
        celix_module_registerService(mod, svcname, (svcVersion), (void*)tmptype, (properties)); \
    } while(0)

/**
 * Register service for the provide module for the provided language.
 * If properties are provided the framework will take ownership.
 *
 * @param mod The module owning the service
 * @param serviceName The service name
 * @param svcVersion The service version. NULL will log a warning
 * @param lang The service language. If NULL 'unspecified' will be used and warning will be logged
 * @param svc The service
 * @param properties Optional properties containing the meta data of the service
 * @return 0 if unsuccessful else the service id
 */
uint64_t celix_module_registerServiceForLang(celix_module* mod, const char* serviceName, const char* svcVersion, const char* lang, void*svc, celix_properties* properties);

/**
 * 'type safe' version of the celix_module_registerServiceForLang function.
 * Which takes as second argument the service type. The type is also used to infer the service name
 */
#define CELIX_MODULE_REGISTER_SERVICE_FOR_LANG(mod, svcType, svcVersion, lang, svc, properties) \
    do { \
        svcType* tmptype = (svc); \
        const char* tmpname = #svcType; \
        celix_module_registerServiceForLang(mod, tmpname, (svcVersion), (lang), (void*)tmptype, (properties)); \
    } while(0)

/**
 * Unregister the service corresponding to the service id for the provided module.
 * Will log an error if the service id is not valid or not owned by the module
 *
 * @param mod The module
 * @param svcId The service id
 */
void celix_module_unregisterService(celix_module* mod, uint64_t svcId);

/**
 * Register service factory for the provide module for the C language.
 * If properties are provided the framework will take ownership.
 *
 * @param mod The module owning the service factory
 * @param serviceName The service name
 * @param serviceVersion The service version. NULL will log a warning
 * @param factory The service factory
 * @param properties Optional properties containing the meta data of the service
 * @return 0 if unsuccessful else the service factory id.
 */
uint64_t celix_module_registerServiceFactory(
        celix_module* mod,
        const char* serviceName,
        const char* serviceVersion,
        celix_service_factory factory,
        celix_properties* properties);

/**
 * Register service factory for the provide module for the C language.
 * If properties are provided the framework will take ownership.
 *
 * @param mod The module owning the service factory
 * @param serviceName The service name
 * @param serviceVersion The service version. NULL will log a warning
 * @param lang The service language. If NULL 'unspecified' will be used and warning will be logged
 * @param factory The service factory
 * @param properties Optional properties containing the meta data of the service
 * @return 0 if unsuccessful else the service factory id.
 */
uint64_t celix_module_registerServiceFactoryForLang(
        celix_module* mod,
        const char* serviceName,
        const char* serviceVersion,
        const char* lang,
        celix_service_factory factory,
        celix_properties* properties);

/**
 * Unregister the service factory corresponding to the service factory id for the provided module.
 * Will log an error if the service factory id is not valid or not owned by the module
 *
 * @param mod The module
 * @param factoryId The service factory id
 */
void celix_module_unregisterServiceFactory(celix_module* mod, uint64_t factoryId);

//service tracker callback function types

typedef int (*celix_serviceTracker_onSettingHighestRankingService_fp)(void* data, void* svc);
typedef int (*celix_serviceTracker_onAddingService_fp)(void* data, void* svc);
typedef int (*celix_serviceTracker_onRemovingService_fp)(void* data, void* svc);

typedef int (*celix_serviceTracker_onSettingHighestRankingServiceWithProperties_fp)(void* data, void* svc, const celix_properties* properties);
typedef int (*celix_serviceTracker_onAddingServiceWithProperties_fp)(void* data, void* svc, const elix_properties* properties);
typedef int (*celix_serviceTracker_onRemovingServiceWithProperties_fp)(void* data, void* svc, const celix_properties* properties);

/**
 * Callbacks wich also provides the owner module as argument. The module is guaranteed to exist during the callback,
 * but should not be stored for later use. The module can be used for extender pattern functionality.
 */
typedef int (*celix_serviceTracker_onSettingHighestRankingServiceWithModule_fp)(void* data, void* svc, const celix_properties* properties, const celix_module* owner);
typedef int (*celix_serviceTracker_onAddingServiceWithModule_fp)(void* data, void* svc, const celix_properties* properties, const celix_module* owner);
typedef int (*celix_serviceTracker_onRemovingServiceWithModule_fp)(void* data, void* svc, const celix_properties* properties, const celix_module* owner);


/**
 * track services for the provided serviceName and/or filter.
 *
 * @param mod The module which will be tracking for the services
 * @param serviceName Optional the service name to tracke
 * @param serviceVersionRange Optional the service version range to track
 * @param filter Optional the LDAP filter to use
 * @param data The data pointer, which will be used in the callbacks
 * @param onSettingFp The callback, which will be called when a new highest ranking service is set.
 * @return the tracker id or 0 if unsuccessful.
 */
uint64_t celix_module_trackService(
        celix_module* mod,
        const char* serviceName,
        const char* serviceVersionRange,
        const char* filter,
        void* data,
        celix_serviceTracker_onSettingHighestRankingService_fp onSettingFp);

/**
 * track services for the provided serviceName and/or filter.
 *
 * @param mod The module which will be tracking for the services
 * @param serviceName Optional the service name to tracke
 * @param serviceVersionRange Optional the service version range to track
 * @param filter Optional the LDAP filter to use
 * @param data The data pointer, which will be used in the callbacks
 * @param onAddingFp The callback, which will be called when service is added.
 * @param onRemovingFp The callback, which will be called when a service is removed
 * @return the tracker id or 0 if unsuccessful.
 */
uint64_t celix_module_trackServices(
        celix_module* mod,
        const char* serviceName,
        const char* serviceVersionRange,
        const char* filter,
        void* data,
        celix_serviceTracker_onAddingService_fp onAddingFp,
        celix_serviceTracker_onRemovingService_fp onRemovingFp);

/**
 * The service tracker options.
 * This struct can be used to configure a more detailed service tracker.
 *
 * This includes service callbacks with properties/module arguments and settings pointers which will be used to
 * lock, set a service, set service properties and set the svcId.
 *
 * All field are optional, and any combination can be used.
 *
 */
typedef struct celix_service_tracker_options {
    //service filter options
    const char* serviceName;
    const char* serviceVersionRange;
    const char* filter;
    const char* lang; //NULL -> 'CELIX_LANG_C'

    //callback options
    void* data;

    celix_serviceTracker_onSettingHighestRankingService_fp onSettingFp;
    celix_serviceTracker_onAddingService_fp onAddingFp;
    celix_serviceTracker_onRemovingService_fp onRemovingFp;

    celix_serviceTracker_onSettingHighestRankingServiceWithProperties_fp onSettingWithProperties;
    celix_serviceTracker_onAddingServiceWithProperties_fp onAddingWithProperties;
    celix_serviceTracker_onRemovingServiceWithProperties_fp onRemovingWithProperties;

    celix_serviceTracker_onSettingHighestRankingServiceWithModule_fp onSettingWithModule;
    celix_serviceTracker_onAddingServiceWithModule_fp onAddingWithModule;
    celix_serviceTracker_onRemovingServiceWithModule_fp onRemovingWithModule;

    //field options
    pthread_mutex_t* svcMutex;
    void** svcField;
    const properties** propertiesField;
    uint64_t* svcIdField;
} celix_service_tracker_options;

//shortcut could be const celix_service_tracker_options opts = {0};
const celix_service_tracker_options CELIX_DEFAULT_SERVICE_TRACKER_OPTIONS {
        .serviceName = NULL,
        .serviceVersionRange = NULL,
        .filter = NULL,
        .lang = NULL,

        .data = NULL,

        .onSettingFp = NULL,
        .onAddingFp = NULL,
        .onRemovingFp = NULL,

        .onSettingWithProperties = NULL,
        .onAddingWithProperties = NULL,
        .onRemovingWithProperties = NULL,

        .onSettingWithModule = NULL,
        .onAddingWithModule = NULL,
        .onRemovingWithModule = NULL,

        .svcMutex = NULL,
        .svcField = NULL,
        .propertiesField = NULL,
        .svcIdField = NULL
};

/**
 * Tracks services using the provided tracker options.
 * The tracker options are only using during this call.
 *
 * @param mod The module which will be tracking for the services.
 * @param opts The tracker options.
 * @return the tracker id or 0 if unsuccessful.
 */
uint64_t celix_module_trackServicesWithOptions(celix_module* mod, const celix_service_tracker_options* opts);

/**
 * Get and lock the current highest ranking service tracker by the provided tracker Id.
 * Note that this also locks the removal of the retrieved service. So if the returned svc
 * is not NULL this should be followed with a celix_module_trackerUngetAndUnlockService call.
 *
 * @param mod The module
 * @param trackerId the id of the tracker to use
 * @return the highest ranking service or NULL if no service is available.
 */
void* celix_module_trackerGetAndLockService(celix_module* mod, uint64_t trackerId);

/**
 * Ungets and unlock the retrieved service.
 * Logs an error if provided module, trackerId, svc or the combination is not valid.
 *
 * @param mod the module
 * @param trackerId the id of the tracker to use
 * @svc the service to unget and unlock
 */
void celix_module_trackerUngetAndUnlockService(celix_module* mod, uint64_t trackerId, void* svc);

/**
 * Get and lock all the currently tracked services.
 * Note that this also locks the removal of the retrieved services. So if the returned services
 * size is not 0, this should be followed with a celix_module_trackerUngetAndUnlockServices call.
 *
 * @param mod The module
 * @param trackerId the id of the tracker to use
 * @param services A array of service pointers to populate
 * @param servicesSize the size of the provided services array
 * @return Number of services added to the services array.
 */
size_t celix_module_trackerGetAndLockServices(celix_module* mod, uint64_t trackerId, void** services, size_t servicesSize);

/**
 * Ungets and unlock the retrieved service
 * Logs an error if provided module, trackerId, services or the combination is not valid.
 *
 * @param mod the module
 * @param trackerId the id of the tracker to use
 * @services the services to unget and unlock
 * @servicesSize the size of the services array
 */
void celix_module_trackerUngetAndUnlockServices(celix_module* mod, uint64_t trackerId, void** services, size_t servicesSize);


/**
 * Stops the tracker and free it's resources.
 * Logs an error in the module, trackerId or combination is not valid.
 *
 * @param mod The module owning the tracker
 * @param trackerId The tracker id
 */
void celix_module_stopTracker(celix_module* mod, uint64_t trackerId);


//service tracker listener callback struct / function types

typedef struct celix_service_tracker_listener_tracker_info {
    uint64_t trackerId;
    const celix_module* owner;
    const char* serviceName;
    const char* filter;
    const char* versionRange;
    const char* lang;
} celix_service_tracker_listener_tracker_info;

/**
 * Callback for handling tracker adding event. The trackerInfo provided is guaranteed to exists and to be valid
 * during the execution of the callback. Note that the provided module pointer should not be stored outside the
 * callback.
 */
typedef int (*celix_serviceTrackerListener_onTrackerAdding_fp)(void* data, const celix_service_tracker_listener_tracker_info* trackerInfo);

/**
 * Callback for handling tracker removing event. The trackerInfo provided is guaranteed to exists and to be valid
 * during the execution of the callback. Note that the provided module pointer should not be stored outside the
 * callback.
 */
typedef int (*celix_serviceTrackerListener_onTrackerRemoving_fp)(void* data, const celix_service_tracker_listener_tracker_info* trackerInfo);

/**
 * Listen for service trackers for the C language.
 * @param mod The module owner of the tracker listener.
 * @param serviceName Optional, only listen to trackers for the specified service name.
 * @param data The data using the in the callbacks.
 * @param onAddingFp The onAdding callbacks.
 * @param onRemovingFp The onRemoving callbacks.
 * @return the service tracker listener id or 0 if unsuccessful.
 */
uint64_t celix_module_listenForTrackers(
        celix_module* mod,
        const char* serviceName,
        void* data,
        celix_serviceTrackerListener_onTrackerAdding_fp onAddingFp,
        celix_serviceTrackerListener_onTrackerRemoving_fp onRemovingFp);

/**
 * Listen for service trackers for the specified language.
 * @param mod The module owner of the tracker listener.
 * @param serviceName Optional, only listen to trackers for the specified service name.
 * @param lang Optional, only listen to trackers for the specified language.
 * @param data The data using the in the callbacks.
 * @param onAddingFp The onAdding callbacks.
 * @param onRemovingFp The onRemoving callbacks.
 * @return the service tracker listener id or 0 if unsuccessful.
 */
uint64_t celix_module_listenForTrackerForLang(
        celix_module* mod,
        const char* serviceName,
        const char* lang,
        void* data,
        celix_serviceTrackerListener_onTrackerAdding_fp onAddingFp,
        celix_serviceTrackerListener_onTrackerRemoving_fp onRemovingFp);

/**
 * Stops the service tracker listener and free it's resources.
 * @param mod The module owning the service tracker listener
 * @param trackerListenerId The id
 */
void celix_module_stopTrackerListener(celix_module* mod, uint64_t trackerListenerId);

/**
 * Lookup and return a pointer to the requested symbol.
 * @param mod The module (library) to use for symbol lookup
 * @param symbol The symbol name
 * @return pointer to the symbol or NULL if unsuccessful.
 */
void* celix_module_sym(celix_module* mod, const char* symbol);

/**
 * Returns a resource entry or NULL if the resource entry cannot be found.
 *
 * Note that resource entries only present as long as the module is installed. So handling module resource
 * entries should only be done by the module self or in callbacks that guarantees that the module is valid during the
 * callback.
 */
FILE* celix_module_resourceEntry(celix_module* mod, const char* path);

/**
 * Store a file in the module store. These files are persistent and can be reused when the framework is restarted.
 * @param mod The module.
 * @param path Relative path to save the entry.
 * @param input The input file which will be copied to the store.
 * @return returns 0 if successful.
 */
int celix_module_storePutFile(celix_module* mod, const char* path, FILE* input);

/**
 * Store content as file in the module store.
 * These files are persistent and can be reused when the framework is restarted.
 * @param mod The module.
 * @param path Relative path to save the entry.
 * @param buf The content to store.
 * @param bufSize the size of the content.
 * @return returns 0 if successful.
 */
int celix_module_storePutContent(celix_module* mod, const char* path, char* buf, size_t bufSize);

/**
 * Returns an store entry as FILE or NULL if the entry cannot be found.
 * Caller should close the returned file.
 */
FILE* celix_module_storeEntry(celix_module* mod, const char* path);

/**
 * Returns the celix_dependency_manager.
 */
celix_dependency_manager_t* celix_module_manager(const celix_module* mod);

//celix/dependency_manager.h

celix_component* celix_dependencyManager_addComponent(celix_dependency_manager* dm, const char* name);
void celix_dependencyManager_removeComponent(celix_dependency_manager* man, celix_component* cmp);

celix_module* celix_dependencyManager_module(const celix_dependency_manager* mng);

//celix/component.h

typedef enum celix_component_state {
    CELIX_COMPONENT_STATE_UNKNOWN                           = 0,
    CELIX_COMPONENT_STATE_INACTIVE                          = 1,
    CELIX_COMPONENT_WAITING_FOR_REQUIRED                    = 2,
    CELIX_COMPONENT_INSTANTIATED_AND_WAITING_FOR_REQUIRED   = 3,
    CELIX_COMPONENT_TRACKING_OPTIONAL                       = 4,
} celix_component_state;

typedef int (*celix_component_lifecycleCallback_fp)(void *handle);

/**
 * Returns the component id or 0 if the
 * provided cmp is invalid.
 */
uint64_t celix_component_id(const celix_component* cmp);


/**
 * Adds a C interface to provide as service if the component is active.
 *
 * @param cmp The component providing th service
 * @param the svc
 * @param serviceName the service name.
 * @param serviceVersion The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 * @returns 0 if successful
 */
int celix_component_addInterface(
        celix_component* cmp,
        void* svc,
        const char* serviceName,
        const char* serviceVersion,
        void* service,
        celix_properties* properties);

/**
 * Adds a C interface to provide as service if the component is active.
 *
 * @param cmp The component providing the service
 * @param lang The language of the service (or C if NULL)
 * @param the svc
 * @param serviceName the service name.
 * @param serviceVersion The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 * @returns 0 if successful
 */
int celix_component_addInterfaceForLang(
        celix_component* cmp,
        const char* lang,
        void* svc,
        const char* serviceName,
        const char* serviceVersion,
        void* service,
        celix_properties properties);

/**
 * Adds a C service factory to provide as service if the component is active.
 *
 * @param cmp The component providing the service factory
 * @param the svc
 * @param serviceName the service name.
 * @param serviceVersion The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param factory The service factory
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 * @returns 0 if successful
 */
int celix_component_addFactory(
        celix_component* cmp,
        const char* serviceName,
        const char* serviceVersion,
        celix_service_factory factory,
        celix_properties properties);

/**
 * Adds a C service factory to provide as service if the component is active.
 *
 * @param cmp The component providing the service factory
 * @param lang The language of the service (or C if NULL)
 * @param the svc
 * @param serviceName the service name.
 * @param serviceVersion The version of the interface (e.g. "1.0.0"), Can be a NULL pointer.
 * @param factory The service factory
 * @param properties To (meta) properties to provide with the service. Can be a NULL pointer.
 * @returns 0 if successful
 */
int celix_component_addFactoryForLang(
        celix_component* cmp,
        const char* lang,
        const char* serviceName,
        const char* serviceVersion,
        celix_service_factory factory,
        celix_properties properties);


/**
 * Set the component context (e.g. user defined data)
 * @param cmp The component
 * @param ctx  The context
 * @return 0 if successful
 */
int celix_component_setContext(celix_component* cmp, celix_component_context* ctx);

/**
 * Returns the conponent context. Note that default this is NULL
 * @param cmp The component
 * @return component context or NULL if not successful
 */
celix_component_context* celix_component_context(const celix_component* cmp);

/**
 * creates and adds a service dependency for the provided component
 * @param component The component which should be the owner of the service dependency
 * @return The service dependency if sucessful else NULL
 */
celix_service_dependency* celix_component_createServiceDependency(celix_component* cmp);

/**
 * Removes and destroys the provided service dependency
 * @param component The owner of the service dependency
 * @return 0 if successful
 */
int celix_component_destroyServiceDependency(celix_component* cmp, celix_service_dependency* dep);

/**
 * Returns the current state of the component
 */
celix_component_state celix_component_currentState(const celix_component* cmp);


/**
 * Returns the DM component name.
 */
const char * celix_component_name(const celix_component* cmp);

/**
 * Returns the dependency manager (owner) of the component
 */
celix_dependency_manager* component_manager(const celix_component* component);

/**
 * Set the component init life cycle callbacks.
 * @param cmp The component which control the lifecycle
 * @param handle The handle used in the callback
 * @param init The callback
 * @return 0 if successful
 */
int celix_component_setInitCallback(celix_component* cmp, void* handle,  celix_component_lifecycleCallback_fp init);

/**
 * Set the component start life cycle callbacks.
 * @param cmp The component which control the lifecycle
 * @param handle The handle used in the callback
 * @param init The callback
 * @return 0 if successful
 */
int celix_component_setStartCallback(celix_component* cmp, void* handle,  celix_component_lifecycleCallback_fp start);

/**
 * Set the component stop life cycle callbacks.
 * @param cmp The component which control the lifecycle
 * @param handle The handle used in the callback
 * @param init The callback
 * @return 0 if successful
 */
int celix_component_setStopCallback(celix_component* cmp, void* handle,  celix_component_lifecycleCallback_fp stop);

/**
 * Set the component deinit life cycle callbacks.
 * @param cmp The component which control the lifecycle
 * @param handle The handle used in the callback
 * @param init The callback
 * @return 0 if successful
 */
int celix_component_setDeinitCallback(celix_component* cmp, void* handle,  celix_component_lifecycleCallback_fp deinit);

/**
 * Set the component init life cycle callbacks macro, sets the callback 'safely'
 */
#define CELIX_COMPONENT_SET_INIT_CALLBACK(cmp, type, handle, fp) \
    do { \
        type* tmptype = (handle); \
        int (*tmp_fp)(type*) = (tmpfp); \
        celix_component_setInitCallback((cmp), (void*)tmptype, (celix_component_lifecycleCallback_fp)tmpfp); \
    } while(0)


/**
 * Set the component start life cycle callbacks macro, sets the callback 'safely'
 */
#define CELIX_COMPONENT_SET_START_CALLBACK(cmp, type, handle, fp) \
    do { \
        type* tmptype = (handle); \
        int (*tmp_fp)(type*) = (tmpfp); \
        celix_component_setStartCallback((cmp), (void*)tmptype, (celix_component_lifecycleCallback_fp)tmpfp); \
    } while(0)

/**
 * Set the component stop life cycle callbacks macro, sets the callback 'safely'
 */
#define CELIX_COMPONENT_SET_STOP_CALLBACK(cmp, type, handle, fp) \
    do { \
        type* tmptype = (handle); \
        int (*tmp_fp)(type*) = (tmpfp); \
        celix_component_setStopCallback((cmp), (void*)tmptype, (celix_component_lifecycleCallback_fp)tmpfp); \
    } while(0)

/**
 * Set the component deinit life cycle callbacks macro, sets the callback 'safely'
 */
#define CELIX_COMPONENT_SET_DEINIT_CALLBACK(cmp, type, handle, fp) \
    do { \
        type* tmptype = (handle); \
        int (*tmp_fp)(type*) = (tmpfp); \
        celix_component_setDeinitCallback((cmp), (void*)tmptype, (celix_component_lifecycleCallback_fp)tmpfp); \
    } while(0)






//celix/service_dependency.h

typedef enum celix_service_dependency_strategy {
    celix_SERVICE_DEPENDENCY_STRATEGY_LOCKING,
    celix_SERVICE_DEPENDENCY_STRATEGY_SUSPEND
} celix_service_dependency_strategy;

typedef int (*celix_service_dependency_callback_fp)(void *handle, void* service);
typedef int (*celix_service_dependency_callbackWithProperties_fp)(void *handle, void* service, celix_properties* properties);

/**
 * Specify if the service dependency is required. default is false
 * @returns 0 if successful
 */
int celix_serviceDependency_setRequired(celix_service_dependency* dep, bool required);

/**
 * Specify if the service dependency update strategy.
 *
 * The CELIX_SERVICE_DEPENDENCY_STRATEGY_LOCKING strategy notifies the component in case the dependencies set
 * changes (e.g. a dependency is added/removed): the component is responsible for protecting via locks
 * the dependencies list and check (always under lock) if the service he's depending on is still available.
 *
 * The CELIX_SERVICE_DEPENDENCY_STRATEGY_SUSPEND (default when no strategy is explicitly set) reliefs the programmer
 * from dealing with service dependencies' consistency issues: in case this strategy is adopted, the component
 * is stopped and restarted (i.e. temporarily suspended) upon service dependencies' changes.
 *
 * Default strategy is celix_SERVICE_DEPENDENCY_STRATEGY_SUSPEND
 * @returns 0 if successful
 */
int serviceDependency_setStrategy(celix_service_dependency* dep ,celix_service_dependency_strategy strategy);


/**
 * Set the service name, version range and filter.
 *
 * @param serviceName The service name. Must have a value.
 * @param serviceVersionRange The service version range, can be a NULL pointer.
 * @param filter The (additional) filter to use (e.g. "(location=front)"). Can be a NULL pointer.
 * @returns 0 if successful
 */
int celix_serviceDependency_setService(celix_service_dependency* dep, const char* serviceName, const char* serviceVersionRange, const char* filter);

/**
 * Sets the 'set' callbacks. This callback is called when a new service is set/removed.
 * If the service is removed the service argument is NULL.
 *
 * @param dep The service dependency which control the callbacks
 * @param handle The handle to provide for the use in the callbacks
 * @param set An optional set callback with only the service as argument
 * @param setWithProps An optional set callback with a service and properties as arguments.
 * @returns 0 if successful
 */
int celix_serviceDependency_setSetCallback(
        celix_service_dependency* dep,
        void* handle,
        celix_service_dependency_callback_fp onSetting,
        celix_service_dependency_callbackWithProperties_fp onSettingWithProps);

//TODO onAdding, onRemoving callbacks

#define CELIX_SERVICE_DEPENDENCY_SET_SET_CALLBACK(dep, handleType, handle, svcType, fp, fpWithProps) \
	do { \
        (handleType)* tmptype = handle; \
        int (*tmpFp)((handleType)*, (svcType)*) = fp; \
        int (*tmpFpWithProps)((handleType)*, (svcType)*, celix_properties*) = fpWithProps; \
		celix_serviceDependency_setSetCallbacks((dep), (void*)tmptype, (celix_service_dependency_callback_fp)fp, (celix_service_dependency_callbackWithProperties_fp)fpWithProps); \
	} while(0)

//TODO onAdding, onRemoving callback macros





//celix/module_activator.h AND celix/module_registration.h

/**
 * For module activation there are two options: the module activator or module registration. Both with their pro and cons.
 * For static module libraries on the the module registration is supported.
 *
 * Options:
 * 1) For the module registration the gcc (also clang supported) constructor (and optional destructor) attributes are
 *    used to call the moduleRegistration functions.
 *
 * 2) If a gcc constructor attribute is not supported for the platform, a set of module activator functions
 *    provided by the module_activator.h header can be implemented.
 *    Implementing this header will result in a set of symbols the framework can use to activate the module.
 *
 * 3) Use the celix_moduleRegistration_registerModule function to register modules programmatically.
 *    For example in a main function.
 */

//celix/module_registration.h

#include <stdbool.h>

typedef int (*celix_module_registration_startModule_fp)(celix_module* mod);
typedef int (*celix_module_registration_stopModule_fp)(celix_module* mod);

typedef struct celix_module_registration_options {
    /**
     * If provided the module registration will take ownership
     * of the properties, set the moduleName and moduleVersion
     * and uses it a the module manifest
     */
    celix_properties* manifest;

    /**
     * If set to true the module will only be installed,
     * not started automatically.
     */
    bool onlyInstall;
} celix_module_registration_options;

//shortcut could be celix_module_registration_options opts = {0};
const celix_module_registration_options CELIX_DEFAULT_MODULE_REGISTRATION_OPTIONS {
        .manifest = NULL,
        .onlyInstall = false
};

/**
 * Registers the module for all current framework and future framework that are created.
 * If no name is provided, the module will not be registered and an error will be logged.
 *
 * When initializing the module, the framework will look for a <module_name>_resourceEntry and
 * <module_name>_resourceEntrySize symbol. If found, the content is assumed to be an embedded zip (or tar?) file.
 * the celix_modules_files CMake command will ensure that these symbols are created and linked to the module library.
 *
 * @param moduleName The name of the module.
 * @param moduleVersion Optional, the version of the module.
 * @startFp Optional the start function which will be called if the module is going to be added and ready to start.
 * @startFp Optional the stop function which will be called if the module going to be removed and ready to start.
 * @opts Optional the additional options. The options will only be used during the registerModule call.
 * @return 0 if successful
 */
int celix_moduleRegistration_registerModule(
        const char* moduleName,
        const char* moduleVersion,
        celix_module_registration_startModule_fp startFp,
        celix_module_registration_stopModule_fp stopFp,
        const celix_module_registration_options* opts);

/* Start example for Option 1 =======================================================================================*/
// my_module.c

#include "celix/services.h" // celix_command_service
#include <stdio.h> //fprintf

struct celix_module_context {
    //user specific
    celix_command_service svc;
    uint64_t svcId;
    int defaultPort = 8080;
};

static int myModule_command(void *handle, char * commandLine, FILE *outStream, FILE *errorStream) {
    fprintf(outStream, "Hello from MyModule\n");
    return 0;
}

static int myModule_start(celix_module* mod) {
    int status = 0;
    celix_module_context* ctx = calloc(1, sizeof(*ctx));
    if (ctx != NULL) {
        celix_module_setContext(mod, ctx);
        svc.handle = ctx; //Not used in this example
        svc.executeCommand = myModule_command;
        ctx->svcId = celix_module_registerService(mod, EXAMPLE_WEB_SERVICE_NAME, EXAMPLE_WEB_SERVICE_VERSION, &act->svc, NULL);
    } else {
        status = 1;
    }
    return status;
}

static int myModule_stop(celix_module* mod) {
    int status = 0;
    celix_module_context* ctx = celix_module_getContext(mod);
    if (ctx != NULL) {
        celix_module_unregisterService(mod, ctx->svcId);
    }
    return status;
}

/* NOTE: static function*/
static void __attribute__((constructor)) myModule_ctor(void) {
    celix_moduleRegistration_register(
            "MyModule",
            "1.0.0",
            myModule_start,
            myModule_stop,
            NULL
    );
}
/* End example for Option 1 =========================================================================================*/

//celix/module_activator.h

/**
 * When the Celix Framework loads modules as shared libraries or bundles, it will lookup the
 * celix_moduleActivator_start and celix_moduleActivator_stop symbols. If present these will be called
 * during starting and stopping the module.
 */

/**
 * Will be called when a module is started.
 * @param mod The module
 * @return 0 if successful
 */
int celix_moduleActivator_start(celix_module* mod);

/**
 * Will be called when a module is stopped.
 * @param mod The module
 * @return 0 if successful
 */
int celix_moduleActivator_stop(celix_module* mod);

//celix/framework_services.h

#include "celix/install_service.h"
#include "celix/module_info_service.h"
#include "celix/service_info_service.h"
#include "celix/service_factory_info_service.h"
#include "celix/service_tracker_info_service.h"
#include "celix/service_tracker_listener_info_service.h"
#include "celix/event_listener_service.h"

/**
 * All framework provided services are using dynFunction_createClosure (ffi) to remove the need for a handle argument.
 */

//celix/install_service.h

typedef struct celix_install_service {
    /**
     * Install a module (shared lib) or bundle (zip)
     * @param Path to a bundle file (ZIP) or shared library
     * @return A module id or 0 if unsuccessful
     */
    uint64_t (*install)(const char* path);

    /**
     * Starts a module if not already started/starting
     * @param moduleId The id of the module to start
     * @return 0 if successful
     */
    int (*start)(uint64_t moduleId);

    /**
     * Stops a module if not already stopped/stopping
     * @param moduleId The id of the module to stop
     * @return 0 if successful
     */
    int (*stop)(uint64_t moduleId);

    /**
     * Uninstall a module and free it's resources
     * @param moduleId The id of the module to uninstall
     * @return 0 if successful
     */
    int (*uninstall)(uint64_t moduleId);

    /**
     * Returns the state of the module.
     * If a module id is invalid CELIX_MODULE_STATE_UNKNOWN will be returned.
     */
    celix_module_state (*state)(uint64_t moduleId);
} celix_install_service;




//celix/module_info_service.h

typedef struct celix_module_info_service {
    /**
     * Returns the installed module ids for the framework.
     * @param arr The array to store the ids
     * @param arrSize The available size of the provided arr
     * @return the actual number ids added to the array
     */
    size_t (*listModuleIds)(uint64_t* arr, size_t arrSize);

    /**
     * Returns a copy of the module name.
     * @param modId the module id
     * @return A copy of the name or NULL if module id is invalid. The name should be released using free.
     */
    char* (*moduleName)(uint64_t modId);

    /**
     * Get the module name and stores it in the provided buffer.
     * at most len characters will be written and a trailing 0 is ensured.
     *
     * @param modId The module id
     * @param buf The string buffer
     * @param len The length of the provider buffer
     * @return number of characters written or 0 if unsuccessful
     */
    int (*getModuleName)(uint64_t modId, char* buf, size_t len);

    /**
     * Returns a copy of the module version.
     * @param modId the module id
     * @return A copy of the version or NULL if module id is invalid. The version should be released using free.
     */
    char* (*moduleVersion)(uint64_t modId);

    /**
     * Get the module version and stores it in the provided buffer.
     * at most len characters will be written and a trailing 0 is ensured.
     *
     * @param modId The module id
     * @param buf The string buffer
     * @param len The length of the provider buffer
     * @return number of characters written or 0 if unsuccessful
     */
    int (*getModuleVersion)(uint64_t modId, char* buf, size_t len);

    /**
     * Returns a copy of the manifest properties for the provided module id.
     * @param modId The module id
     * @return A copy of the manifest properties or NULL if the module id is invalid.
     */
    celix_properties* (*moduleManifest)(uint64_t modId);

    size_t (*listServices)(uint64_t modId, uint64_t* arr, size_t arrSize);
    size_t (*listServiceFactories)(uint64_t modId, uint64_t* arr, size_t arrSize);
    size_t (*listTrackers)(uint64_t modId, uint64_t* arr, size_t arrSize);
    size_t (*listTrackerListeners)(uint64_t modId, uint64_t* arr, size_t arrSize);
    size_t (*listComponents)(uint64_t modId, uint64_t* arr, size_t arrSize);
} celix_module_info_service;



//celix/service_info_service.h

typedef struct celix_service_info {
    uint64_t serviceId;
    uint64_t moduleOwner;
    char* serviceName;
    char lang[16];
    uintt64_t rank;
    celix_properties* properties;
    size_t usageCount;
    uint64_t factoryId;
    struct timeval registrationTime;

    struct {
        uint64_t usingModule;
        struct timeval trackTime;
    }* usages;
    size_t usagesSize;
} celix_service_info;

typedef struct celix_service_info_service {
    /**
     * Returns the all provided service ids for the framework.
     * @param arr The array to store the ids
     * @param arrSize The available size of the provided arr
     * @return the actual number ids added to the array
     */
    size_t (*listServices)(uint64_t* arr, size_t arrSize);

    /**
     * Populates the service info for the provided service id.
     * Caller needs to release the serviceName,
     * properties and usages.
     * Or call releaseInfo.
     *
     * If the service is the result of a service factory the
     * factoryId will be set (not 0).
     *
     * If the service id is invalid, all fields will be 0.
     *
     * @param serviceId The service id
     * @param info The service info struct to fill
     */
    void (*getInfo)(uint64_t* serviceId, celix_service_info* info);

    /**
    * Convenience function to release (free) the content in the info struct
    */
    void (*releaseInfo)(celix_service_info* info);
} celix_service_info_service;



//celix/service_factory_info_service.h

typedef struct celix_service_factory_info {
    uint64_t factoryId;
    uint64_t moduleOwner;
    char* serviceName;
    char lang[16];
    celix_properties* properties;
    size_t usageCount;
    struct timeval registrationTime;

    struct {
        uint64_t serviceId;
        uint64_t boundedToModule;
        struct timeval creationTime;
    }* boundedServices;
    size_t boundedServicesSize;
} celix_service_factory_info;

typedef struct celix_service_factory_info_service {
    /**
     * Returns the all provided factory ids for the framework.
     * @param arr The array to store the ids
     * @param arrSize The available size of the provided arr
     * @return the actual number ids added to the array
     */
    size_t (*listFactories)(uint64_t* arr, size_t arrSize);

    /**
     * Populates the service factory info for the provided factory id.
     * Caller needs to release the serviceName, properties and
     * boundedServices.
     * Or call releaseInfo.
     *
     * If the factory id is invalid, all fields will be 0.
     *
     * @param factoryId The factory id
     * @param info The service factory info struct to fill
     */
    void (*getInfo)(uint64_t* factoryId, celix_service_factory_info* info);

    /**
     * Convenience function to release (free) the content in the info struct
     */
    void (*releaseInfo)(celix_service_factory_info* info);
} celix_service_factory_info_service;



//celix/service_tracker_info_service.h

typedef struct celix_service_tracker_info {
    uint64_t trackerId;
    uint64_t moduleOwner;
    char* serviceName;
    char* filter;
    char lang[16];

    struct {
        uint64_t serviceId;
        uintt64_t rank;
        struct timeval trackedTime;
    }* trackedServices; /*ranked*/
    size_t trackedServicesSize;
} celix_service_tracker_info;

typedef struct celix_tracker_info_service {
    /**
     * Returns the all provided service tracker ids for the framework.
     * @param arr The array to store the ids
     * @param arrSize The available size of the provided arr
     * @return the actual number ids added to the array
     */
    size_t (*listServiceTrackers)(uint64_t* arr, size_t arrSize);

    /**
     * Populates the service tracker info for the provided service tracker id.
     * Caller needs to release the serviceName, filter and
     * trackedServices.
     * Or call releaseInfo.
     *
     * If the service tracker id is invalid, all fields will be 0.
     *
     * @param trackerId The service tracker id
     * @param info The service tracker info struct to fill
     */
    void (*getInfo)(uint64_t* trackerId, celix_service_tracker_info* info);

    /**
     * Convenience function to release (free) the content in the info struct
     */
    void (*releaseInfo)(celix_service_tracker_info* info);
} celix_tracker_info_service;


//celix/service_tracker_listener_info_service.h

typedef struct celix_tracker_listener_info_service {
    //TODO
} celix_tracker_listener_info_service;



//celix/component_info_service.h

typedef struct celix_component_info {
    char* componentName;
    uint64_t ownerModuleId;
    celix_component_state state;

    //service dependencies
    struct {
        uint64_t trackerId;
        bool required;
        size_t availableCount;
        char* filter;
    }* dependencies;
    size_t dependenciesSize;

    //provided service
    struct {
        uint64_t serviceId;
        celix_properties* properties;
    }* providedServices;
    size_t providedServicesSize;

    //provided factories
    struct {
        uint64_t factoryId;
        celix_properties* properties;
    }* providedFactories;
    size_t providedFactoriesSize;

} celix_component_info;

typedef struct celix_component_info_service {
    /**
     * Returns the all provided components ids for the framework.
     * @param arr The array to store the ids
     * @param arrSize The available size of the provided arr
     * @return the actual number ids added to the array
     */
    size_t (*listFactories)(uint64_t* arr, size_t arrSize);

    /**
     * Populates the component info for the provided factory id.
     * Caller needs to release the componentName, dependencies filter,
     * dependencies, providedServices properties,  providedServices,
     * providedFactories properties, providedFactories.
     * Or call releaseInfo.
     *
     * If the component id is invalid, all fields will be 0.
     *
     * @param componentId The component id
     * @param info The component info struct to fill
     */
    void (*getInfo)(uint64_t* factoryId, celix_component_info* info);

    /**
     * Convenience function to release (free) the content in the info struct
     */
    void (*releaseInfo)(celix_component_info* info);
} celix_component_info_service;



//celix/event_service.h

#include "celix/events.h"

/**
 * A Celix Event. Events always have a topic and
 * optional can have multiple key/value entries.
 *
 * For the event entries, the key is always a string and
 * the value pointer to something.
 *
 * Important to note is that events (and their entries) are only quaranteed to exists
 * during the handle of the event callback. As result event and their entries should not
 * be stored for later use.
 * The values of pointer to primitive types can of course be copied (e.g. a module id).
 */
typedef struct celix_event {
    const char* eventTopic;
    const celix_map* entries;
};

#define CELIX_EVENT_TOPICS      "event.topics"
#define CELIX_EVENT_FILTER      "event.filter"

/**
 * The celix_event_listener is a generic whiteboard service which can be registered
 * to handle events for specific topics.
 *
 * If this service is registered with the event.topics property, only those topics (comma seperated) will trigger the
 * onEvent callback.
 * If this service is registered with the event.filter propertie, only event which match the filter, using the
 * string entries of celix_event.entries as filter target, will trigger the onEvent callback.
 *
 */
typedef struct celix_event_listener {
    void* handle;

    /**
     * Will be called when a matching event is triggered.
     *
     * @param handle the service handle
     * @param event The event, which will quaranteed to exists during the execution of the onEvent callbacks
     */
    void (*onEvent)(void* handle, const celix_event* event);
} celix_event_listener;


typedef void (*freeEvent_fp)(celix_event* event);

typedef struct celix_event_admin {
    void* handle;

    /**
     * Posts an event. The event will be send and freed async.
     */
    void (*postEvent)(void* handle, const char* topic, celix_event* event, freeEvent_fp freeEvent);

    /**
     * Send an event. This call will block til the event propagated through all registered event listener.
     * The event can be freed after the sendEvent function returns.
     */
    void (*sendEvent)(void* handle, const char* topic, const celix_event* event);
} celix_event_admin;



//celix/events.h


/**
 * A Module INSTALL, START, STOP or UNINSTALL Event will be created by the framework
 * - if a event admin service is available - when installing, starting, stopping and uninstalling a module.
 *
 * The event map has two entries:
 *   - A module id uint64 entry, containing the moduleId
 *   - A module const ptr entry, containing a celix_module*
 *
 *   The module entry can be used to for extender pattern functionality and dynamic lookup functionality
 *   (e.g. like gogo commands). The module entry can only be used in during a handleEvent callback,
 *   after the callback module entry can be invalid.
 */

#define CELIX_EVENT_MODULE_TOPIC_ALL        "celix/event/module/*"
#define CELIX_EVENT_MODULE_TOPIC_INSTALL    "celix/event/module/INSTALL"
#define CELIX_EVENT_MODULE_TOPIC_START      "celix/event/module/START"
#define CELIX_EVENT_MODULE_TOPIC_STOP       "celix/event/module/STOP"
#define CELIX_EVENT_MODULE_TOPIC_UNINSTALL  "celix/event/module/UNINSTALL"

#define CELIX_EVENT_MODULE_KEY_MODULE "module"
#define CELIX_EVENT_MODULE_KEY_MODULE_ID "moduleId"


/**
 * A Service REGISTER/UNREGISTER Event will be created by the framework - if a event admin service is available -
 * when registering, unregistering a service
 *
 * The event map has the following entries
 *   - A serviceId uint64 entry, containing the service id
 *   - A serviceName string entry, containing the service name
 *   - A moduleOwnerId uint64 entry, containing the module owner id
 *   - A owner const ptr entry containing a celix_module*
 *   - A properties const ptr entry containing a celix_properties* entry.
 *
 *   The owner entry can be used to for extender pattern functionality and dynamic lookup functionality
 *   (e.g. like gogo commands). The module entry can only be used in during a handleEvent callback,
 *   after the callback module entry can be invalid.
 */

#define CELIX_EVENT_SERVICE_TOPIC_ALL           "celix/event/service/*"
#define CELIX_EVENT_SERVICE_TOPIC_REGISTER      "celix/event/service/REGISTER"
#define CELIX_EVENT_SERVICE_TOPIC_UNREGISTER    "celix/event/service/UNREGISTER"

#define CELIX_EVENT_SERVICE_KEY_SERVICE_ID      "serviceId"
#define CELIX_EVENT_SERVICE_KEY_SERVICE_NAME    "serviceName"
#define CELIX_EVENT_SERVICE_KEY_MODULE_OWNER_ID "moduleOwnerId"
#define CELIX_EVENT_SERVICE_KEY_OWNER           "owner"
#define CELIX_EVENT_SERVICE_KEY_PROPERTIES      "properties"

#ifdef __cplusplus
}
#endif

#endif //CELIX_H_
