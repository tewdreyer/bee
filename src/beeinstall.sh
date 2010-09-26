#!/bin/bash

###############################################################################
###############################################################################
###############################################################################

VERSION=0.1

: ${BEEFAULTS=/etc/bee.defaults}

if [ -e ${BEEFAULTS} ] ; then
    . ${BEEFAULTS}
fi

: ${METADIR:=/usr/share/bee}
: ${PKGSTORE:=/usr/src/bee/pkgs}


##### iee_usage ###############################################################
iee_usage() {
    echo "bee_install v${VERSION} 2009-2010"
    echo ""
    echo "  by Tobias Dreyer and Marius Tolzmann <{dreyer,tolzmann}@molgen.mpg.de>"
    echo ""
    echo " Usage: $0 [action] [options] <pkg>"
    echo ""
    echo " action:"
    echo "   -i | --install    install package (default action)"
    echo "   -u | --upgrade    install package"
    echo "   -h | --help       display this help.. 8)"
    echo ""
    echo " options:"
    echo "   -f | --force      can be used to force installation come what may"
    echo "   -v | --verbose    bee more verbose (can be used twice e.g. -vv)"
    echo
}

###############################################################################
##
##
pkg_install_all() {
    for pkg in ${@} ; do
        pkg_install ${pkg}
    done
}

###############################################################################
##
##

#sub1-sub2-subn-name-V.V.V.V-R.A.iee.tar.bz2

get_fullversion_from_pkg() {
    echo $(echo $1 | sed -e 's,^\(.*\)-\(.*\)-\(.*\)\.\(.*\)$,\2-\3,' - )
}

###############################################################################
##
##
get_name_from_pkg() {
    echo $(echo $1 | sed -e 's,^\(.*\)-\(.*\)-\(.*\)$,\1,' - )
}

###############################################################################
### pkg_install ###
##
## IN: search-pattern
##
## OUT: ...
##
## DESCRIPTION: ...
##
pkg_install() {
    search=${1}
    
    # install specific package
    
    if [ -f "${search}" ] ; then
         do_install "${search}"
    fi
    
    # install package from repository
    
    if [ -f "${PKGSTORE}/${search}" ] ; then
         do_install "${PKGSTORE}/${search}"
    fi
    
    for e in "" ".$(arch)" ".noarch" ".any" ; do
        if [ -f "${PKGSTORE}/${search}${e}.iee.tar.bz2" ] ; then
	    do_install "${PKGSTORE}/${search}${e}.iee.tar.bz2"
	fi
    done
    
    avail=$(get_pkg_list_repository "${search}")
    
    print_pkg_list ${avail}
}


###############################################################################
### get_installed_versions ###
##
## IN: full_packagename
##
## OUT: list of installed packages matching pkgname(full_packagename)
##
## DESCRIPTION: ...
##
get_installed_versions() {
    local pkg=${1}
    
    local pname=$(get_name_from_pkg ${pkg})
    local list
    
    for i in $(get_pkg_list_installed "${pname}") ; do
        local installed=$(get_name_from_pkg ${i})
	if [ "${installed}" = "${pname}" ] ; then
	    list="${list:+${list} }${i}"
	fi
    done
        
    echo "${list}"
}

###############################################################################
### check_installed ###
##
## IN: full_packagename
##
## OUT: n/a - only returns if package is not installed in any version
##
## DESCRIPTION:
##     - print installed versions of "full_packagename"
##     - exit(2) if installed version of "full_packagename" exists
##
check_installed() {
    local pkgname=${1}
    
    local installed=$(get_installed_versions ${pkgname})
    
    if [ "${installed}" != "" ] ; then
        for i in ${installed} ; do
	    local v=$(get_fullversion_from_pkg ${i})
	    local n=$(get_name_from_pkg ${i})
	    
	    if [ ${i} = ${pkgname} ] ; then
	        echo "[  already installed  ] ${n} - version ${v}"
	    else
	        echo "[alternative installed] ${n} - version ${v}"
	    fi
	done
	exit 2;
    fi
}

###############################################################################
### do_install ###
##
## IN: filename
##
## OUT: n/a
##
## DESCRIPTION:
##     - calls check_installed which exits if package is already installed
##       (skip test if -f option was given on commandline)
##     - installs package
##
do_install() {
    local file=${1}
    local pkgname=$(basename $(basename $(basename ${file} .tar.gz) .tar.bz2) .iee)
    
    if [ "${OPT_F}" = "0" ] ; then
        check_installed ${pkgname}
    fi
    
    # create bee-filename
    eval $(tar -xOPf /tmp/bash-4.0-0.i686.iee.tar.bz2 META)
    BEE="${PNF}-${PVF}-${PR}.bee"
    unset PNF PVF PR PGRP
    
#    echo "would install ${file}"
#    exit 0
    
#    mkdir -p ${METADIR}/${pkgname}
    tar -xvvPf ${file} \
          --transform="s,FILES,${METADIR}/${pkgname}/FILES," \
          --transform="s,${BEE},${METADIR}/${pkgname}/${BEE}," \
          --transform="s,META,${METADIR}/${pkgname}/META," \
          --show-transformed-names
    exit $?
}


###############################################################################
### get_pkg_list ###
##
## IN: 
##
##
##
get_pkg_list_repository() {
    local search=${1}
    
    local hits arch
    
    if [ ${OPT_A} -le '1' ] ; then
        arch="\.(any|noarch|$(arch))"
    fi
    
    hits=$(find ${PKGSTORE} -mindepth 1 \
             | egrep "${arch}\.iee" \
	     | egrep "${search}" \
	     | sort)

    echo ${hits}
}

###############################################################################
### get_pkg_list_installed ###
##
## IN: 
##
##
##
get_pkg_list_installed() {
    local search=${1}
    
    local hits arch
    
    hits=$(find ${METADIR} -maxdepth 1 -mindepth 1 -type d -printf "%f\n" \
             | egrep "${search}")
    
    echo ${hits}
}

###############################################################################
### print_pkg_list ###
##
## args: $LIST
##
##
##
print_pkg_list() {
    local -i f=0
    
    if [ ! "${1}" ] ; then
        echo "no matching packages found in repository.."
	exit 4
    fi
    
    for p in ${@} ; do
	local pkgname=$(basename $(basename ${p} .tar.bz2) .iee)
	
	local installed=$(get_installed_versions ${pkgname})
	
	local status
	for i in ${installed} ; do
	    if [ ${i} = ${pkgname} ] ; then
	        status="*"
	    else
	        status=${status:--}
	    fi
	done
	status=${status:- }
	if [ "${status}" = " " -o ${OPT_A} -gt 0 ] ; then
	    echo " [${status}] ${pkgname}"
	    f=$f+1
	fi
	unset status
    done
    
    echo "${f} of ${#} packages displayed matching search criteria."
    
    if [ ${f} -eq 0 ] ; then
        echo "no new packages found in repository. try -v to list already installed packages."
	exit 5
    fi
}

###############################################################################
###############################################################################
###############################################################################


options=$(getopt -n bee_install \
                 -o iavfuh \
                 --long install,upgrade,verbose,all,force,help \
                 -- "$@")
if [ $? != 0 ] ; then
  iee_usage
  exit 1
fi
eval set -- "${options}"

declare -i OPT_A=0
declare -i OPT_F=0

while true ; do
  case "$1" in
    -a|--all|-v|--verbose)
      shift;
      OPT_A=$OPT_A+1
      ;;
    -f|--force)
      shift;
      OPT_F=$OPT_F+1
      ;;
    -u|--upgrade)
      shift
      pkg_upgrade $@
      exit 0
      ;;    
    -h|--help)
      iee_usage
      exit 0
    ;;
    -i|--install)
      shift
      ;;
    *)
      shift
      if [ -z ${1} ] ; then
           iee_usage
	   exit 1
      fi
      pkg_install_all ${@}
      exit 0;
      ;;
  esac
done