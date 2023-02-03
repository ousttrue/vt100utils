c = meson.get_compiler('c')

deps = [vt100utils_dep]
if host_machine.system() != 'windows'
    deps += c.find_library('m')
endif

# executable(
#     'hover',
#     'hover.cpp',
#     'tui.cpp',
#     install: true,
#     dependencies: deps,
# )

executable(
    'overflow',
    'overflow.cpp',
    'tui.cpp',
    install: true,
    dependencies: deps,
)

executable(
    'words',
    'words.cpp',
    'tui.cpp',
    install: true,
    dependencies: deps,
)

# executable(
#     'truecolor_stresstest',
#     'truecolor_stresstest.cpp',
#     'tui.cpp',
#     install: true,
#     dependencies: deps,
# )
