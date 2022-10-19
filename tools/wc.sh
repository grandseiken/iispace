#!/bin/bash
git ls-files | grep "\.\(cc\|h\|bzl\|glsl\|yml\|proto\|sh\|vdf\)$\|BUILD$" | xargs wc -l
