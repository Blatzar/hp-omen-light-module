make -C $(nix-build -E '(import <nixpkgs> {}).linux.dev' --no-out-link)/lib/modules/*/build M=$(pwd) modules
sudo insmod ./hp-color.ko
sudo rmmod hp_color
dmesg | tail -n 4