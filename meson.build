project('Aegisub', ['c', 'cpp'],
        license: 'BSD-3-Clause',
        meson_version: '>=0.50.0',
        # c_std=c11 should be here but it breaks Linux builds for some reason, needs investigation
        default_options: ['cpp_std=c++11', 'b_lto=false'],
        version: '3.2.2')

cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

is_msvcwin = (cxx.get_id() == 'msvc') and (host_machine.system() == 'windows')
is_gccwin = (cxx.get_id() == 'gcc') and (host_machine.system() == 'windows')
is_linux = (host_machine.system() == 'linux')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
localedir = prefix / get_option('localedir')
docdir = prefix / 'doc'
dataroot = datadir / 'aegisub'
add_project_arguments('-DP_DATA="@0@"'.format(dataroot), language: 'cpp')

if host_machine.system() == 'windows'
    add_project_arguments('-DNOMINMAX', '-D_WIN32_WINNT=0x0602', language: 'cpp')
	add_project_arguments('-DUNICODE', language: 'cpp')
endif

conf = configuration_data()
conf.set_quoted('P_DATA', dataroot)
conf.set('WITH_UPDATE_CHECKER', get_option('enable_update_checker'))

deps = []
deps_inc = []

if host_machine.system() != 'windows'
    deps += dependency('fontconfig')
endif

if (is_gccwin)
	deps += cxx.find_library('libws2_32')
endif


deps += cc.find_library('m', required: false)
deps += cc.find_library('dl', required: false)

deps += dependency('libass', version: '>=0.9.7', fallback: ['libass', 'libass_dep'])

if (is_gccwin)
	deps += cc.find_library('iconv')
endif

deps += dependency('boost', version: '>=1.50.0',
                   modules: ['chrono', 'filesystem', 'locale', 'regex',
                             'system', 'thread'])
deps += dependency('zlib', fallback: ['zlib', 'zlib_dep'])

# wxWidgets
deps += dependency('wxWidgets', version: '>=3.0.0', modules: ['std', 'stc', 'gl'])

# icu
deps += dependency('icu-uc', version: '>=4.8.1.1')
deps += dependency('icu-i18n', version: '>=4.8.1.1', required: cxx.get_id() != 'msvc')

dep_avail = []
foreach dep: [
    # audio, in order of precedence
    ['libpulse', '', 'PulseAudio'],
    ['alsa', '', 'ALSA'],
    ['portaudio-2.0', '', 'PortAudio'],
    ['openal', '>=0.0.8', 'OpenAL'],
    # other
    ['fftw3', '', 'FFTW3'],
    ['hunspell', '', 'Hunspell'], # needs a proper port
    ['uchardet', '', 'uchardet'], # needs a proper port
]
    d = dependency(dep[0], version: dep[1] != '' ? dep[1]: '>=0',
                   required: false)

    optname = dep[0].split('-')[0]
    if d.found() and not get_option(optname).disabled()
        deps += d
        conf.set('WITH_@0@'.format(dep[0].split('-')[0].to_upper()), '1')
        dep_avail += dep[2]
    elif get_option(optname).enabled()
        error('@0@ enabled but not found'.format(dep[2]))
    endif
endforeach

deps += dependency('ffms2')

# DirectSound
if is_msvcwin and not get_option('directsound').disabled()
    dsound_dep = cc.find_library('dsound', required: get_option('directsound'))
    winmm_dep = cc.find_library('winmm', required: get_option('directsound'))
    ole32_dep = cc.find_library('ole32', required: get_option('directsound'))
    have_dsound_h = cc.has_header('dsound.h')
    if not have_dsound_h and get_option('directsound').enabled()
        error('DirectSound enabled but dsound.h not found')
    endif
    if dsound_dep.found() and winmm_dep.found() and ole32_dep.found() and have_dsound_h
        deps += [dsound_dep, winmm_dep, ole32_dep]
        conf.set('WITH_DIRECTSOUND', '1')
		add_project_arguments('-DWITH_DIRECTSOUND', language: 'cpp')
        dep_avail += 'DirectSound'
    endif
endif

# PortAudio
if (is_gccwin)
	deps += dependency('portaudio-2.0')
	dep_avail += 'PortAudio'
	add_project_arguments('-DWITH_PORTAUDIO', language: 'cpp')
endif


def_audio = get_option('default_audio_output')
if def_audio != 'auto'
    if not dep_avail.contains(def_audio)
        error('Default audio output "@0@" selected but not available'.format(def_audio))
    endif
elif dep_avail.length() != 0
    def_audio = dep_avail[0]
else
    def_audio = ''
endif

conf_platform = configuration_data()
conf_platform.set('DEFAULT_PLAYER_AUDIO', def_audio)

dep_gl = dependency('gl', required: false)
if not dep_gl.found()
    if host_machine.system() == 'windows'
        dep_gl = cc.find_library('opengl32', required: false)
    else
        dep_gl = cc.find_library('GL', required: false)
    endif

    if not cc.has_header('GL/gl.h')
        dep_gl = dependency('', required: false)
    endif
endif

if not dep_gl.found()
    error('OpenGL implementation not found')
endif

deps += dep_gl

# TODO: csri

acconf = configure_file(output: 'acconf.h', configuration: conf)

subdir('libaegisub')
subdir('src')
