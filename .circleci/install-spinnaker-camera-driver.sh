#!/bin/bash

install-spinnaker-camera-driver() {
  set -e
  local LSB_RELEASE=$(lsb_release -cs)
  local ARCH=$(dpkg --print-architecture)
  if [ "$LSB_RELEASE" = "bionic" ]; then
    local SPINNAKER_DOWNLOAD_URL='https://drive.google.com/uc?id=1gFWVVNmheAncxKw4A8pffkMkFaQYchQe'
  elif [ "$LSB_RELEASE" = "focal" ]; then
    local SPINNAKER_DOWNLOAD_URL='https://drive.google.com/uc?id=1fInCHM-cH644ixUIesANfO4idvjaAg7x'
  fi
  local SPINNAKER_VERSION="2.3.0.77"
  local SPINNAKER_HEADER_DIR=/opt/spinnaker/include
  local SPINNAKER_ARCHIVE_DIR="/tmp/spinnaker-${SPINNAKER_VERSION}-${ARCH}.tar.gz"
  local SPINNAKER_INSTALLER_DIR="/tmp/spinnaker-${SPINNAKER_VERSION}-${ARCH}"
  local SPINNAKER_RULE_FILE=/etc/udev/rules.d/40-flir-spinnaker.rules
  local SPINNAKER_PROFILE_FILE=/etc/profile.d/spinnaker.sh

  if [ -e $SPINNAKER_HEADER_DIR/Spinnaker.h ]; then
    echo "Spinnaker camera driver is already installed"
    return 0
  fi

  echo "Downloading driver $SPINNAKER_VERSION for $LSB_RELEASE $ARCH"

  if [ ! -e $SPINNAKER_ARCHIVE_DIR ]; then
    if [ "$(which gdown)" = "" ]; then
      pip install -U gdown
    fi

    gdown -q "$SPINNAKER_DOWNLOAD_URL" -O $SPINNAKER_ARCHIVE_DIR

    if [ ! -e $SPINNAKER_ARCHIVE_DIR ]; then
      echo "Failed to download spinnaker driver: $SPINNAKER_DOWNLOAD_URL"
      return 1
    fi
  else
    echo "Using already downloaded archive"
  fi

  if [ ! -d $SPINNAKER_INSTALLER_DIR ]; then
    echo "Unarchiving"

    (cd /tmp && tar zxvf $SPINNAKER_ARCHIVE_DIR)

    if [ ! -d $SPINNAKER_INSTALLER_DIR ]; then
      echo "No directory found in $SPINNAKER_INSTALLER_DIR. Unarchiving failed?"
      return 1
    fi
  else
    echo "Using already unarchived installer"
  fi

  echo "Accepts license"
  echo libspinnaker libspinnaker/accepted-flir-eula boolean true | debconf-set-selections

  echo "Installing"
  cd $SPINNAKER_INSTALLER_DIR
  for i in {0..3}; do
    if dpkg -i libspin*.deb; then break; fi
    apt-get install -f -qq -y
  done
  for i in {0..3}; do
    if dpkg -i spinview-qt-*.deb; then break; fi
    apt-get install -f -qq -y
  done
  for i in {0..3}; do
    if dpkg -i spinupdate-*.deb; then break; fi
    apt-get install -f -qq -y
  done
  for i in {0..3}; do
    if dpkg -i spinnaker-*.deb; then break; fi
    apt-get install -f -qq -y
  done

  echo "Installing udev rules"
  if [ ! -e $SPINNAKER_RULE_FILE ]; then
    mkdir -p $(dirname $SPINNAKER_RULE_FILE)
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1e10", MODE:="0666"' > $SPINNAKER_RULE_FILE

    systemctl restart udev || true
  else
    echo "udev rule file is already installed"
  fi

  echo "Installing profile"
  bash ./configure_spinnaker_paths.sh
  bash ./configure_gentl_paths.sh 64

  echo "Done"

  return 0
}

install-spinnaker-camera-driver
