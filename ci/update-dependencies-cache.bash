#!/usr/bin/env nix-shell
#! nix-shell -i bash ./default.nix
set -eu

# (re)build up-to-date CI batsim package, push it on binary cache
nix-build ci/default.nix | cachix push batsim

# Build up-to-date batsim_dev package, push it on binary cache
nix-build ${DATAMOVEPKGS:-~/datamovepkgs} -A batsim_dev | cachix push batsim
