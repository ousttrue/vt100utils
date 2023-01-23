c = meson.get_compiler('c')
m_dep = c.find_library('m')

executable(
    'hover',
    'hover.c',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'overflow',
    'overflow.c',
    install: true,
    dependencies: vt100utils_dep,
)

executable(
    'words',
    'words.c',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)

executable(
    'truecolor_stresstest',
    'truecolor_stresstest.c',
    install: true,
    dependencies: [vt100utils_dep, m_dep],
)
