add_subdirectory(hello_world)
add_subdirectory(mongoose)

#add_subdirectory(receiver)
#add_subdirectory(receiver-2.0)
#add_subdirectory(sender)

#add_subdirectory(echo_service/client)
#add_subdirectory(echo_service/server)
add_subdirectory(whiteboard)
#add_subdirectory(echo_service)

deploy("test" BUNDLES shell shell_tui log_service chapter04-correct-lookup)