{
  description = "vicc emulates a smart card and makes it accessible through PC/SC";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-stable.url = "github:NixOS/nixpkgs/nixos-23.11";
  };
  
  outputs = inputs@{ self, nixpkgs, flake-utils, ... }:
    with flake-utils.lib; eachSystem allSystems (system:
    let
      pkgs = import nixpkgs { inherit system; };
      pkgs-stable = import inputs.nixpkgs-stable { inherit system; };
    in rec {
      packages = rec {

        vicc = pkgs.stdenv.mkDerivation {
          name = "vicc";
          src = self;
          buildInputs = with pkgs; [ 
            pcsclite 
          ];
          propagatedBuildInputs = with pkgs; [
            (python3.withPackages (ps: with ps; [ 
              pyscard 
              pillow
              readline
              pbkdf2
              pycryptodome
            ]))
            qrencode
          ];
          nativeBuildInputs = with pkgs; [
            autoconf 
            automake 
            libtool 
            pkg-config 
            help2man 
            python3Packages.setuptools
            python3Packages.wrapPython
          ];
          buildPhase = ''
            autoreconf -vfi
            ./configure --prefix=$out --sysconfdir=$out/etc
            make
          '';
          installPhase = ''
            make install
          '';
          postFixup = ''
            wrapProgram "$out/bin/vicc" \
              --prefix PYTHONPATH : "$PYTHONPATH" \
              --prefix PYTHONPATH : "$out/${pkgs.python3.sitePackages}" \
              --prefix PATH : "${pkgs.python3}/bin"
          '';
        };

      };
      
      shell = pkgs.mkShell {
        buildInputs = with pkgs; [ autoconf automake libtool pkg-config help2man pcsclite python3 ];
      };

      defaultPackage = packages.vicc;
    });
}