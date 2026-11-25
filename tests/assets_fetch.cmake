# ROBO-261: the build-time asset fetch (cmake/RV2Assets.cmake) against local fixtures, no network:
# cmake -DSOURCE=<repo> -DSCRATCH=<dir> -DPYTHON=<python3> -P tests/assets_fetch.cmake
# robo_art installs a hash-checked pack with VERSION, reuses its cache and skips a wrong hash; original installs only
# when every Content/main hash matches; url needs main/pack.tsv that passes check-pack.py; a failed download or broken
# archive warns and installs nothing; none removes every pack; release installs refuse original files.
cmake_minimum_required(VERSION 3.25)
include("${SOURCE}/cmake/RV2Assets.cmake")

set(dir "${SCRATCH}/assets-fetch")
file(REMOVE_RECURSE "${dir}")
set(out "${dir}/runtime/packs")
set(cache "${dir}/cache")
set(failures 0)

macro(fail what)
  message(SEND_ERROR "FAIL: ${what}")
  math(EXPR failures "${failures} + 1")
endmacro()
macro(check_exists path what)
  if(NOT EXISTS "${path}")
    fail("${what}")
  endif()
endmacro()
macro(check_missing path what)
  if(EXISTS "${path}")
    fail("${what}")
  endif()
endmacro()

function(file_url path out)
  file(TO_CMAKE_PATH "${path}" p)
  if(p MATCHES "^/")
    set(${out} "file://${p}" PARENT_SCOPE)
  else()
    set(${out} "file:///${p}" PARENT_SCOPE) # C:/...
  endif()
endfunction()

# The fixture art pack (ROBO-264) as a release archive
execute_process(COMMAND "${PYTHON}" -B "${SOURCE}/tests/art_pack_fixture.py" "${SOURCE}/content/base-pack.tsv"
                "${dir}/art" RESULT_VARIABLE rc OUTPUT_QUIET)
if(NOT rc EQUAL 0)
  fail("the fixture art pack builds")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar czf "${dir}/rv2-art-9.tar.gz" main WORKING_DIRECTORY "${dir}/art")
file(SHA256 "${dir}/rv2-art-9.tar.gz" art_sha)
file_url("${dir}/rv2-art-9.tar.gz" art_url)
file_url("${dir}/never-there.tar.gz" missing_url_early)

# robo_art: installed with VERSION
rv2_fetch_assets(KINDS robo_art OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}"
                 ART_URL "${art_url}" ART_SHA256 "${art_sha}" ART_VERSION 9)
check_exists("${out}/robo_art/main/pack.tsv" "robo_art installs main/pack.tsv")
file(STRINGS "${out}/robo_art/VERSION" version)
if(NOT version STREQUAL "9")
  fail("robo_art writes VERSION")
endif()

# A reconfigure with the same pin does nothing: the stamp matches, so neither the cache nor the pack is touched
string(SUBSTRING "${art_sha}" 0 12 short)
file(REMOVE "${cache}/rv2-art-9-${short}.tar.gz")
file(WRITE "${out}/robo_art/marker" "kept\n")
rv2_fetch_assets(KINDS robo_art OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}"
                 ART_URL "${missing_url_early}" ART_SHA256 "${art_sha}" ART_VERSION 9)
check_exists("${out}/robo_art/marker" "an unchanged pin leaves the installed pack alone")
check_missing("${cache}/rv2-art-9-${short}.tar.gz" "an unchanged pin downloads nothing")

# Rebuild from scratch with the source archive gone: the cache serves it
file(REMOVE_RECURSE "${out}") # refills the cache first
rv2_fetch_assets(KINDS robo_art OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}"
                 ART_URL "${art_url}" ART_SHA256 "${art_sha}" ART_VERSION 9)
file(RENAME "${dir}/rv2-art-9.tar.gz" "${dir}/moved.tar.gz")
file(REMOVE_RECURSE "${out}")
rv2_fetch_assets(KINDS robo_art OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}"
                 ART_URL "${art_url}" ART_SHA256 "${art_sha}" ART_VERSION 9)
check_exists("${out}/robo_art/main/pack.tsv" "a clean build reuses the cached archive")

# A wrong hash and an unreachable URL install nothing and don't stop CMake
file(REMOVE_RECURSE "${out}")
file(RENAME "${dir}/moved.tar.gz" "${dir}/rv2-art-9.tar.gz")
rv2_fetch_assets(KINDS robo_art OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}"
                 ART_URL "${art_url}" ART_SHA256 "0000000000000000000000000000000000000000000000000000000000000000" ART_VERSION 10)
check_missing("${out}/robo_art" "a wrong hash installs nothing")
file_url("${dir}/missing.tar.gz" missing_url)
rv2_fetch_assets(KINDS robo_art OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}"
                 ART_URL "${missing_url}" ART_SHA256 "${art_sha}" ART_VERSION 11)
check_missing("${out}/robo_art" "an unreachable URL installs nothing")

# url: a pack one folder down in the archive, checked by check-pack.py
file(MAKE_DIRECTORY "${dir}/url/mypack")
file(COPY "${dir}/art/main" DESTINATION "${dir}/url/mypack")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${dir}/mypack.zip" --format=zip mypack WORKING_DIRECTORY "${dir}/url")
file_url("${dir}/mypack.zip" pack_url)
rv2_fetch_assets(KINDS url OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" URL "${pack_url}" PYTHON "${PYTHON}")
check_exists("${out}/url/main/pack.tsv" "url installs a pack found one folder down")
file(MAKE_DIRECTORY "${dir}/nopack/main")
file(WRITE "${dir}/nopack/main/readme.txt" "no manifest\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar czf "${dir}/nopack.tar.gz" main WORKING_DIRECTORY "${dir}/nopack")
file_url("${dir}/nopack.tar.gz" nopack_url)
file(REMOVE_RECURSE "${out}/url")
rv2_fetch_assets(KINDS url OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" URL "${nopack_url}" PYTHON "${PYTHON}")
check_missing("${out}/url" "a pack without pack.tsv is not installed")
file(MAKE_DIRECTORY "${dir}/badtsv/main")
file(WRITE "${dir}/badtsv/main/pack.tsv" "id\tfile\n../x\t../x.png\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar czf "${dir}/badtsv.tar.gz" main WORKING_DIRECTORY "${dir}/badtsv")
file_url("${dir}/badtsv.tar.gz" badtsv_url)
rv2_fetch_assets(KINDS url OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" URL "${badtsv_url}" PYTHON "${PYTHON}")
check_missing("${out}/url" "a pack that fails check-pack.py is not installed")
# An unpinned archive with a member that leaves the folder, or a link, writes nothing at all (tools/safe-extract.py)
execute_process(COMMAND "${PYTHON}" -I -c "
import io, sys, tarfile
for path, kind in ((sys.argv[1], 'dotdot'), (sys.argv[2], 'link')):
    with tarfile.open(path, 'w:gz') as t:
        good = tarfile.TarInfo('main/pack.tsv'); data = b'id\\n'; good.size = len(data); t.addfile(good, io.BytesIO(data))
        if kind == 'dotdot':
            bad = tarfile.TarInfo('../escaped.txt'); bad.size = 1; t.addfile(bad, io.BytesIO(b'x'))
        else:
            bad = tarfile.TarInfo('main/out'); bad.type = tarfile.SYMTYPE; bad.linkname = '../../..'; t.addfile(bad)
" "${dir}/dotdot.tar.gz" "${dir}/link.tar.gz")
foreach(evil dotdot link)
  file_url("${dir}/${evil}.tar.gz" evil_url)
  rv2_fetch_assets(KINDS url OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" URL "${evil_url}" PYTHON "${PYTHON}")
  check_missing("${out}/url" "an archive with a ${evil} member is installed")
endforeach()
check_missing("${out}/escaped.txt" "a '..' member is written outside the folder")
check_missing("${out}/url.partial" "a refused archive leaves its scratch folder")

file(WRITE "${dir}/broken.zip" "not an archive\n")
file_url("${dir}/broken.zip" broken_url)
rv2_fetch_assets(KINDS url OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" URL "${broken_url}" PYTHON "${PYTHON}")
check_missing("${out}/url" "a broken archive is not installed and does not stop CMake")

# original: a fake upstream zip whose files don't match tools/original-assets.sha256 is refused
set(commit "0123456789abcdef0123456789abcdef01234567")
set(inner "BaboViolent2-${commit}/BaboViolent2/Content/main")
file(WRITE "${dir}/up/${inner}/bv2.cfg" "not the original\n")
file(WRITE "${dir}/up/${inner}/extra.txt" "unlisted\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${dir}/up.zip" --format=zip "BaboViolent2-${commit}"
                WORKING_DIRECTORY "${dir}/up")
file_url("${dir}/up.zip" up_url)
rv2_fetch_assets(KINDS original OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" ORIGINAL_URL "${up_url}" COMMIT "${commit}"
                 PYTHON "${PYTHON}")
check_missing("${out}/original" "original data that fails the hash list is not installed")
check_missing("${cache}/BaboViolent2-${commit}.zip" "a bad original archive leaves the cache")

# original: the same zip with a hash list that matches it is installed as packs/original/main
file(SHA256 "${dir}/up/${inner}/bv2.cfg" cfg_hash)
file(WRITE "${dir}/up-hashes.sha256" "${cfg_hash}  BaboViolent2/Content/main/bv2.cfg\n")
rv2_fetch_assets(KINDS original OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" ORIGINAL_URL "${up_url}" COMMIT "${commit}"
                 ORIGINAL_HASHES "${dir}/up-hashes.sha256" PYTHON "${PYTHON}")
check_exists("${out}/original/main/bv2.cfg" "original data that matches the hash list is installed as original/main")
check_missing("${out}/original/main/extra.txt" "a file the hash list doesn't name is not installed")

# Combined kinds, then none removes every pack and touches no URL
rv2_fetch_assets(KINDS "robo_art;url" OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" URL "${pack_url}" PYTHON "${PYTHON}"
                 ART_URL "${art_url}" ART_SHA256 "${art_sha}" ART_VERSION 9)
check_exists("${out}/robo_art/VERSION" "robo_art;url installs the art pack")
check_exists("${out}/url/main/pack.tsv" "robo_art;url installs the url pack")
rv2_fetch_assets(KINDS none OUT "${out}" CACHE "${cache}" SOURCE "${SOURCE}" URL "${missing_url}")
foreach(kind robo_art url original)
  check_missing("${out}/${kind}" "none removes ${kind}")
endforeach()

# Release installs: a file with an original asset's hash is found (the repo has none, so the test hashes its own file)
file(WRITE "${dir}/install/share/bv2/main/clean.txt" "clean\n")
file(WRITE "${dir}/install/share/bv2/main/sounds/fake.wav" "pretend original\n")
file(SHA256 "${dir}/install/share/bv2/main/sounds/fake.wav" fake_hash)
rv2_original_hashes("${SOURCE}" hashes)
list(LENGTH hashes n)
if(NOT n GREATER 300)
  fail("the hash list loads")
endif()
file(GLOB_RECURSE installed "${dir}/install/*")
rv2_original_files_in("${installed}" "${hashes}" found)
if(found)
  fail("a clean install passes")
endif()
rv2_original_files_in("${installed}" "${hashes};${fake_hash}" found)
if(NOT found MATCHES "fake.wav$")
  fail("a file with an original hash is found")
endif()

if(failures GREATER 0)
  message(FATAL_ERROR "${failures} check(s) failed")
endif()
message(STATUS "ok: assets fetch")
