project('vt100utils', ['c', 'cpp'], default_options: ['cpp_std=c++20'])
add_project_arguments(['-stdlib=libc++'], language: 'cpp')
add_project_link_arguments(['-stdlib=libc++', '-lstdc++fs'], language: 'cpp')

vt100utils_dep = declare_dependency(
    include_directories: include_directories('.'),
)

subdir('demos')
