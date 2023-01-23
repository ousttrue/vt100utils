c = meson.get_compiler('c')
m_dep = c.find_library('m')

executable(
    'hover',
    'hover.cpp',
    'tuibox.cpp',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'overflow',
    'overflow.cpp',
    'tuibox.cpp',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'words',
    'words.cpp',
    'tuibox.cpp',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)

executable(
    'truecolor_stresstest',
    'truecolor_stresstest.cpp',
    'tuibox.cpp',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)
