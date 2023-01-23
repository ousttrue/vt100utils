c = meson.get_compiler('c')
m_dep = c.find_library('m')

executable(
    'hover',
    'hover.cpp',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'overflow',
    'overflow.cpp',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'words',
    'words.cpp',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)

executable(
    'truecolor_stresstest',
    'truecolor_stresstest.cpp',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)
