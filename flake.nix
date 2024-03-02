{
  description = "BerkeleyDB based SQL Database";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    hsql-parser = {
      url = "github:scrufulufugus/sql-parser";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, hsql-parser, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages = rec {
          sql5300 = pkgs.callPackage ./. { hsql-parser = hsql-parser.packages.${system}.default; };
          default = sql5300;
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          packages = with pkgs; [ gnumake ];
        };
      });
}
