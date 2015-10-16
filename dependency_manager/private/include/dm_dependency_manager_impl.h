
#ifndef CELIX_DM_DEPENDENCY_MANAGER_IMPL_H
#define CELIX_DM_DEPENDENCY_MANAGER_IMPL_H

struct dm_dependency_manager {
    array_list_pt components;

    pthread_mutex_t mutex;
};

#endif //CELIX_DM_DEPENDENCY_MANAGER_IMPL_H
