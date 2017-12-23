set -e


function get_absolute_path()
{
  python -c "import os.path; print os.path.abspath('$1')"
}

function get_relative_path() # $1 - path to a file, $2 - base dir
{
  python -c "import os.path; print os.path.relpath('$1', os.path.dirname('$2'))"
}

function update_links_in_boost_libs_in_libs() # $1 - path to boost libs, $2 - path to libs folder
{
  install_name_tool -change libboost_system.dylib          $1/libboost_system.dylib          $2/libboost_thread.dylib
  install_name_tool -change libboost_system.dylib          $1/libboost_system.dylib          $2/libboost_filesystem.dylib
  install_name_tool -change libboost_system.dylib          $1/libboost_system.dylib          $2/libboost_locale.dylib
  install_name_tool -change libboost_system.dylib          $1/libboost_system.dylib          $2/libboost_chrono.dylib

}

function update_links_in_boost_libs_in_binary() # $1 - path to boost libs, $2 - binary to update
{
  if [ -z "$1" ] || [ -z "$2" ]
  then
    echo "update_links_in_boost_libs_in_binary is called with no or bad argumens"
    return 1
  fi
  install_name_tool -change libboost_atomic.dylib           $1/libboost_atomic.dylib           $2
  install_name_tool -change libboost_program_options.dylib  $1/libboost_program_options.dylib  $2
  install_name_tool -change libboost_date_time.dylib        $1/libboost_date_time.dylib        $2
  install_name_tool -change libboost_chrono.dylib           $1/libboost_chrono.dylib           $2
  install_name_tool -change libboost_regex.dylib            $1/libboost_regex.dylib            $2
  install_name_tool -change libboost_system.dylib           $1/libboost_system.dylib           $2
  install_name_tool -change libboost_filesystem.dylib       $1/libboost_filesystem.dylib       $2
  install_name_tool -change libboost_thread.dylib           $1/libboost_thread.dylib           $2
  install_name_tool -change libboost_serialization.dylib    $1/libboost_serialization.dylib    $2
  install_name_tool -change libboost_locale.dylib           $1/libboost_locale.dylib           $2
  return 0
}

# return immediately if this script was called from another script
[ "$(basename $0)" != "$(basename $BASH_SOURCE)" ] && return


if [[ $# -ne 2 ]]; then
  echo "error: wrong args"
  echo "usage: $0 <path to boost library dir> <path to binary>"
  echo "       -- update links lib paths in binary and libs to relative-to-binary paths, using given lib dir as final lib folder"
  exit 1
fi

path_to_library=$1
path_to_bin=$2
relative_bin_to_lib=$(get_relative_path $path_to_library $path_to_bin)
update_links_in_boost_libs_in_binary @executable_path/$relative_bin_to_lib $path_to_bin
update_links_in_boost_libs_in_libs @executable_path/$relative_bin_to_lib $path_to_library

exit 0
