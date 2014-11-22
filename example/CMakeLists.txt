set(EXAMPLE_SRC
    Example.cpp)

add_executable(example ${EXAMPLE_SRC})
target_link_libraries(example ltbl)
target_link_libraries(example ${SFML_LIBRARIES} ${GLEW_LIBRARY} ${SFML_DEPENDENCIES} ${OPENGL_LIBRARIES})
