#!/usr/bin/env python3
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""Generate a Conan lockfile and CycloneDX 1.6 SBOM for Apache Celix.

This is not a custom SBOM generator. It resolves the same Conan dependency
graph as ``conan lock create`` and writes a CycloneDX 1.6 document using
Conan 2's built-in ``conan.tools.sbom.cyclonedx_1_6`` (the function also
used by the built-in ``cyclone_1.6`` deployer).

The official deployer is only invoked by ``conan install``, which requires
binary packages. This helper uses the resolved recipe graph so CI can
produce a lockfile and SBOM without compiling Celix or its dependencies.

The graph is the canonical full-feature Linux/Release Conan configuration
used by Celix CI (``build_all=True``, C++17, shared libraries). It is not
a union of every possible Celix build.
"""

from __future__ import annotations

import argparse
import json
import os
import sys


# Meaningful options from .github/workflows/conan_create.yml (Linux/Release).
# Intentionally omitted: enable_ccache (CI-only), enable_testing / sanitizers
# (test graph, not the product graph), and celix_install_deprecated_api
# (not a current recipe option).
CANONICAL_OPTIONS = [
    "celix/*:build_all=True",
    "celix/*:celix_cxx17=True",
    "mosquitto/*:broker=True",
    "*:shared=True",
]

SBOM_FILENAME = "sbom-cyclonedx-1.6.json"
LOCKFILE_FILENAME = "conan.lock"
EXPECTED_HOST_PACKAGES = ("openssl", "zlib", "libzip", "libuv", "jansson", "libcurl")


def _require_cyclonedx():
    try:
        from conan.tools.sbom import cyclonedx_1_6
    except ImportError as exc:
        raise SystemExit(
            "Conan 2.x with built-in CycloneDX 1.6 support is required "
            "(conan.tools.sbom.cyclonedx_1_6). Install a current Conan 2 "
            "release, e.g. `pip install -U conan`.\n"
            f"Import error: {exc}"
        ) from exc
    return cyclonedx_1_6


def _resolve_graph(conanfile_dir, options):
    from conan.api.conan_api import ConanAPI

    api = ConanAPI()
    cwd = os.path.abspath(conanfile_dir)
    path = api.local.get_conanfile_path(".", cwd, py=True)

    try:
        profile_host = api.profiles.get_profile(
            ["default"], settings=["build_type=Release"], options=list(options)
        )
        profile_build = api.profiles.get_profile(["default"])
    except Exception as exc:
        raise SystemExit(
            "Failed to load the Conan default profile. Run "
            "`conan profile detect` first, matching Celix CI "
            "(compiler.cppstd=gnu17).\n"
            f"{exc}"
        ) from exc

    remotes = api.remotes.list()
    lockfile = api.lockfile.get_lockfile(
        lockfile=None, conanfile_path=path, cwd=cwd, partial=True
    )
    graph = api.graph.load_graph_consumer(
        path, None, None, None, None,
        profile_host, profile_build, lockfile,
        remotes, False,
    )
    graph.report_graph_error()
    api.graph.analyze_binaries(
        graph, None, remotes=remotes, update=False, lockfile=lockfile
    )
    lockfile = api.lockfile.update_lockfile(lockfile, graph, False, clean=True)
    return api, graph, lockfile


def _write_sbom(graph, output_dir, cyclonedx_1_6):
    sbom = cyclonedx_1_6(graph.root.conanfile)
    if isinstance(sbom, str):
        data = json.loads(sbom)
        text = sbom if sbom.endswith("\n") else sbom + "\n"
    else:
        data = sbom
        text = json.dumps(sbom, indent=2) + "\n"

    sbom_path = os.path.join(output_dir, SBOM_FILENAME)
    with open(sbom_path, "w", encoding="utf-8") as handle:
        handle.write(text)
    return sbom_path, data


def verify_sbom(data):
    errors = []
    if data.get("bomFormat") != "CycloneDX":
        errors.append(f"bomFormat is {data.get('bomFormat')!r}, expected 'CycloneDX'")
    if data.get("specVersion") != "1.6":
        errors.append(f"specVersion is {data.get('specVersion')!r}, expected '1.6'")

    component = (data.get("metadata") or {}).get("component") or {}
    root_name = str(component.get("name") or "")
    if "celix" not in root_name.lower():
        errors.append(f"metadata.component.name is {root_name!r}, expected it to contain 'celix'")

    components = data.get("components") or []
    if not components:
        errors.append("SBOM has no dependency components")

    names = {str(item.get("name") or "") for item in components}
    missing = [pkg for pkg in EXPECTED_HOST_PACKAGES if pkg not in names]
    if missing:
        errors.append(f"SBOM is missing expected host packages: {', '.join(missing)}")

    if errors:
        raise SystemExit("SBOM sanity check failed:\n- " + "\n- ".join(errors))

    return {
        "root": root_name,
        "component_count": len(components),
        "component_names": sorted(names),
    }


def parse_args(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--conanfile-dir",
        default=".",
        help="Directory containing conanfile.py (default: current directory)",
    )
    parser.add_argument(
        "--output-dir",
        default="sbom-out",
        help="Directory for conan.lock and the CycloneDX JSON (default: sbom-out)",
    )
    parser.add_argument(
        "-o",
        "--option",
        action="append",
        dest="options",
        help="Override canonical Conan options. If omitted, the canonical "
             "Linux/Release full-feature options are used.",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    cyclonedx_1_6 = _require_cyclonedx()
    options = args.options if args.options else CANONICAL_OPTIONS
    output_dir = os.path.abspath(args.output_dir)
    os.makedirs(output_dir, exist_ok=True)

    api, graph, lockfile = _resolve_graph(args.conanfile_dir, options)
    api.lockfile.save_lockfile(lockfile, LOCKFILE_FILENAME, output_dir)
    lock_path = os.path.join(output_dir, LOCKFILE_FILENAME)
    sbom_path, data = _write_sbom(graph, output_dir, cyclonedx_1_6)
    summary = verify_sbom(data)

    print(f"Wrote lockfile: {lock_path}")
    print(f"Wrote SBOM:     {sbom_path}")
    print(f"Root component: {summary['root']}")
    print(f"SBOM components ({summary['component_count']}):")
    for name in summary["component_names"]:
        print(f"  - {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
