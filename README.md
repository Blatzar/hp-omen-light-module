Small personal Linux module for turning off the chassis light
on HP-TG01 desktops. May work on more HP products.

You can enable blinking or breathing, but
that requires you modify the code.

Tons of code taken from: https://github.com/pelrun/hp-omen-linux-module

### Installation on NixOS

In your hardware.nix:

```nix
{ ... }:

let
    hp-color = pkgs.callPackage (builtins.fetchTarball {
        url = "https://github.com/Blatzar/hp-omen-light-module/archive/refs/tags/1.0.tar.gz";
        sha256 = "sha256:0nn61hl1y9z5k03hcd3mpv1srs01sxibnq09278rzrhgv15mp33m";
    }) { };
in {
  boot.extraModulePackages = [ hp-color ]; # Make it available
  boot.kernelModules = [ "hp-color" ]; # Load it
  # Rest of your config here
}
```
