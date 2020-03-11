project('Aegisub', ['c', 'cpp'],
        license: 'BSD-3-Clause',
        meson_version: '>=0.50.0',
        # c_std=c11 should be here but it breaks Linux builds for some reason, needs investigation
        default_options: ['cpp_std=c++11', 'b_lto=true'],
        version: '3.2.2')

cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

is_gccwin = (cxx.get_id() == 'gcc') and (host_machine.system() == 'windows')
is_linux = (host_machine.system() == 'linux')

if (is_gccwin)
	local_path = get_option('local_path')
	if local_path == ''
		error('No local_path specified')
	endif
endif

version_sh = find_program('tools/version.sh')
version_inc = include_directories('.')
version_h = custom_target('git_version.h',
                          command: [version_sh, meson.source_root()],
                          build_by_default: true,
                          build_always_stale: true, # has internal check whether target file will be refreshed
                          output: ['git_version.h', 'git_version.xml'])

if host_machine.system() == 'darwin' and get_option('build_osx_bundle')
    prefix = meson.current_build_dir() / 'Aegisub.app' / 'Contents'
    bindir = prefix / 'MacOS'
    datadir = prefix / 'SharedSupport'
    localedir = prefix / 'Resources'
else
    prefix = get_option('prefix')
    bindir = prefix / get_option('bindir')
    datadir = prefix / get_option('datadir')
    localedir = prefix / get_option('localedir')
endif
docdir = prefix / 'doc'
dataroot = datadir / 'aegisub'
add_project_arguments('-DP_DATA="@0@"'.format(dataroot), language: 'cpp')

if host_machine.system() == 'windows'
    add_project_arguments('-DNOMINMAX', '-D_WIN32_WINNT=0x0602', language: 'cpp')
endif

conf = configuration_data()
conf.set_quoted('P_DATA', dataroot)
if get_option('credit') != ''
    conf.set_quoted('BUILD_CREDIT', get_option('credit'))
endif
conf.set('WITH_UPDATE_CHECKER', get_option('enable_update_checker'))

deps = []
deps_inc = []

if host_machine.system() == 'darwin'
    add_languages('objc', 'objcpp')
    add_project_arguments('-DGL_SILENCE_DEPRECATION', language: 'cpp')
    # meson does not currently support objcpp_std
    add_project_arguments('-std=c++11', language: 'objcpp')
elif host_machine.system() != 'windows'
    deps += dependency('fontconfig')
endif


deps += cc.find_library('m', required: false)
deps += cc.find_library('dl', required: false)
iconv_dep = cc.find_library('iconv', required: false)
if not iconv_dep.found() and host_machine.system() == 'windows'
    iconv_sp = subproject('iconv') # this really needs to be replaced with a proper port
    deps_inc += iconv_sp.get_variable('iconv_incs')
    iconv_dep = iconv_sp.get_variable('libiconv_dep')
endif
deps += iconv_dep

deps += dependency('libass', version: '>=0.9.7',
                   fallback: ['libass', 'libass_dep'])
deps += dependency('boost', version: '>=1.50.0',
                   modules: ['chrono', 'filesystem', 'locale', 'regex',
                             'system', 'thread'])
deps += dependency('zlib', fallback: ['zlib', 'zlib_dep'])

# wxWidgets
if (is_linux)
    deps += dependency('wxWidgets', version: '>=3.0.0', modules: ['std', 'stc', 'gl'])
elif (is_gccwin)
	deps_inc += include_directories(local_path / 'wxWidgets-3.1.3/include')
	deps_inc += include_directories(local_path / 'wxWidgets-3.1.3/lib/vc14x_x64_dll/mswud')
	add_project_arguments('-DUNICODE', '-D_UNICODE', language: 'cpp')
	add_project_arguments('-DwxMSVC_VERSION=14x', '-DWXUSINGDLL', language: 'cpp')
endif

# icu
if (is_linux)
	deps += dependency('icu-uc', version: '>=4.8.1.1')
	deps += dependency('icu-i18n', version: '>=4.8.1.1', required: cxx.get_id() != 'msvc')
elif (is_gccwin)
	deps += cxx.find_library('icuuc', dirs: local_path / 'mingw-w64-x86_64-icu-65.1-1-any.pkg/mingw64/lib')
	deps += cxx.find_library('icuin', dirs: local_path / 'mingw-w64-x86_64-icu-65.1-1-any.pkg/mingw64/lib')
endif

dep_avail = []
foreach dep: [
    # audio, in order of precedence
    ['libpulse', '', 'PulseAudio'],
    ['alsa', '', 'ALSA'],
    ['portaudio-2.0', '', 'PortAudio'],
    ['openal', '>=0.0.8', 'OpenAL'],
    # video
    ['ffms2', '', 'FFMS2'], # needs a proper port
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

if host_machine.system() == 'windows' and not get_option('directsound').disabled()
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
        dep_avail += 'DirectSound'
    endif
endif

if host_machine.system() == 'darwin'
    frameworks_dep = dependency('appleframeworks', modules : ['CoreText', 'CoreFoundation'])
    deps += frameworks_dep
endif

# TODO: OSS

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
if host_machine.system() == 'darwin'
    conf.set('HAVE_OPENGL_GL_H', 1)
endif

if not dep_gl.found()
    error('OpenGL implementation not found')
endif

deps += dep_gl

# TODO: csri

acconf = configure_file(output: 'acconf.h', configuration: conf)

subdir('libaegisub')
subdir('packages')
subdir('po')
subdir('src')
