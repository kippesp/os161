# setenv.sh - project workflow shortcuts

export OS161=$HOME/projects/ops-class-os161
export OS161_SRC=$OS161/os161.git
export PATH=$OS161/tools/bin:$PATH
export OS161_SYSROOT=$OS161/root

export CURRENT_KERNEL=DUMBVM
#export CURRENT_KERNEL=ASST1
#export CURRENT_KERNEL=ASST2
#export CURRENT_KERNEL=ASST3

# set tags file created by 'bmake tags'
export TAGSFILE=$OS161_SRC/kern/compile/$CURRENT_KERNEL/tags

# configure gdb for SYS161 connection
gdb() {
    pushd $OS161_SYSROOT
    os161-gdb --eval-command="target remote unix:.sockets/gdb" kernel
    popd
}

# go to main source dir
gosrc() {
    cd $OS161_SRC
}

# go to sys/root dir
gosys() {
    cd $OS161_SYSROOT
}

# go to kernel dir
gokern() {
    cd $OS161_SRC/kern
}

# buildkernel
bk() {
    pushd $OS161_SRC/kern/compile/$CURRENT_KERNEL
    bmake
    bmake tags

    if [ $? -eq 0 ]; then
        bmake install
    fi
    popd
}

# build userspace tools
btools() {
    pushd $OS161_SRC
    bmake

    if [ $? -eq 0 ]; then
        bmake install
    fi
    popd
}

# start sys161 kernel
st() {
    pushd $OS161_SYSROOT
    sys161 kernel
    popd
}

# only required to remake defs.mk (to set the root path info)
# and satisfy sys161.conf config file dependency.
os161config() {
    pushd $OS161_SRC
    ./configure --ostree=$OS161_SYSROOT

    if [ ! -e $OS161_SYSROOT ]; then
        mkdir $OS161_SYSROOT
    fi

    if [ ! -e $OS161_SYSROOT/sys161.conf ]; then
        pushd $OS161_SYSROOT
        ln -s $OS161_SRC/sys161.conf
    fi

    popd
}

kernelconfig() {
    cd $OS161_SRC/kern/conf
    ./config $1
}

# help
h() {
    echo "COMMANDS for OS161/SYS161 (kernel: $CURRENT_KERNEL):"
    echo ""
    echo "os161config       - initial config for fresh code"
    echo "kernelconfig CFG  - configure the kernel for kern/conf/CFG"
    echo "bk                - build kernel"
    echo ""
    echo "gosrc  - change to source dir"
    echo "gosys  - change to OS161_ROOT dir"
    echo "gokern - change to OS161_SRC/kern dir"
    echo ""
    echo "btools - build userspace tools"
    echo "st     - start sys161 kernel VM"
    echo ""
    echo "gdb    - start os161-gdb and connect to SYS161"
    echo ""
    echo "h      - show this help"
}

# Export my functions

export -f os161config
export -f kernelconfig
export -f gosrc
export -f gosys
export -f gokern
export -f bk
export -f btools
export -f st
export -f h
export -f gdb

# config new conf.kern with:
#   cd ~/projects/ops-class-os161/os161.git/kern/conf
#   ./config ASST1
#   ./config ASST2
#   ./config DUMBVM
