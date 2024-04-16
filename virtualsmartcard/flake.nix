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
          name = "parse-ts";
          src = self;
          buildInputs = with pkgs; [ autoconf automake libtool pkg-config help2man pcsclite ];
          buildPhase = ''
            autoreconf -vfi
            ./configure --prefix=$out
            make
          '';
          installPhase = ''
            make install
          '';
        };

      };
      
      shell = pkgs.mkShell {
        buildInputs = with pkgs; [ autoconf automake libtool pkg-config help2man pcsclite ];
      };

      defaultPackage = packages.vicc;
    });
}