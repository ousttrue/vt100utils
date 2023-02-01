c = meson.get_compiler('c')
m_dep = c.find_library('m')

executable(
    'hover',
    'hover.cpp',
    'tui.cpp',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'overflow',
    'overflow.cpp',
    'tui.cpp',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'words',
    'words.cpp',
    'tui.cpp',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)

executable(
    'truecolor_stresstest',
    'truecolor_stresstest.cpp',
    'tui.cpp',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)
