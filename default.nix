{ stdenv, lib, fetchFromGitHub, kmod, nukeReferences, linux, linuxPackages, kernel ? linuxPackages.kernel }:

stdenv.mkDerivation rec {
  name = "hp-color-${version}-${kernel.version}";
  version = "1.0";

  src = ./.;

  kernelVersion = kernel.modDirVersion;
  nativeBuildInputs = kernel.moduleBuildDependencies;
  buildInputs = [ nukeReferences ];

  buildPhase = ''
    make -C ${linux.dev}/lib/modules/*/build modules "M=$(pwd)"
  '';

  installPhase = ''
    mkdir -p $out/lib/modules/$kernelVersion/misc
    for x in $(find . -name '*.ko'); do
      nuke-refs $x
      cp $x $out/lib/modules/$kernelVersion/misc/
    done
  '';

  meta = with lib; {
    description = "A kernel module for turning off the chassis light on HP-TG01 desktops.";
    homepage = "https://github.com/Blatzar/hp-omen-light-module";
    license = licenses.gpl3;
    platforms = platforms.linux;
  };
}
