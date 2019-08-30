#!/bin/bash

install-flycapture2() {
  set -e
  set -x
  local LSB_RELEASE=$(lsb_release -cs)
  local ARCH=$(dpkg --print-architecture)
  local FLYCAPTURE_VERSION="2.13.3.31"
  local FLYCAPTURE_HEADER_DIR=/usr/include/flycapture
  local FLYCAPTURE_ARCHIVE_DIR="/tmp/flycapture2-${FLYCAPTURE_VERSION}-${ARCH}-Ubuntu$(lsb_release -rs)-pkg.tar.gz"
  local FLYCAPTURE_INSTALLER_DIR="/tmp/flycapture2-${FLYCAPTURE_VERSION}-${ARCH}"

  if [ -e $FLYCAPTURE_HEADER_DIR/Flycapture2.h ]; then
    echo "Flycapture camera driver is already installed"
    return 0
  fi

  local FLYCAPTURE_DOWNLOAD_URL=
  case $LSB_RELEASE in
    bionic)
      FLYCAPTURE_DOWNLOAD_URL="https://drive.google.com/uc?id=1fMcCnxjyAaqF3oUn1Ze3MSVHxikxO0x6"
      ;;
  esac

  echo "Downloading driver $FLYCAPTURE_VERSION for $LSB_RELEASE $ARCH"

  if [ ! -e $FLYCAPTURE_ARCHIVE_DIR ]; then
    if [ "$(which gdown)" = "" ]; then
      pip install -U gdown
    fi

    gdown "$FLYCAPTURE_DOWNLOAD_URL" -O $FLYCAPTURE_ARCHIVE_DIR

    if [ ! -e $FLYCAPTURE_ARCHIVE_DIR ]; then
      echo "Failed to download flycapture driver: $FLYCAPTURE_DOWNLOAD_URL"
      return 1
    fi
  else
    echo "Using already downloaded archive"
  fi

  if [ ! -d $FLYCAPTURE_INSTALLER_DIR ]; then
    echo "Unarchiving"

    (cd /tmp && tar zxvf $FLYCAPTURE_ARCHIVE_DIR)

    if [ ! -d $FLYCAPTURE_INSTALLER_DIR ]; then
      echo "No directory found in $FLYCAPTURE_INSTALLER_DIR. Unarchiving failed?"
      return 1
    fi
  else
    echo "Using already unarchived installer"
  fi

  echo "Installing"
  cd $FLYCAPTURE_INSTALLER_DIR
  for i in {0..3}; do
    if dpkg -i libflycapture*.deb; then break; fi
    apt-get install -f -qq -y
  done
  for i in {0..3}; do
    if dpkg -i libmultisync-*.deb; then break; fi
    apt-get install -f -qq -y
  done
  for i in {0..3}; do
    if dpkg -i flycap*.deb; then break; fi
    apt-get install -f -qq -y
  done
  for i in {0..3}; do
    if dpkg -i updatorgui*.deb; then break; fi
    apt-get install -f -qq -y
  done

  echo "Done"

  return 0
}

install-flycapture2
