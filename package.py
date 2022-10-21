from spack.package import *


class SeismCore(CMakePackage):
    """An HDF5 parallel IO performance testing harness"""

    homepage = "https://github.com/frankwillmore/seism-core"
    git = "https://github.com/frankwillmore/seism-core.git"

    maintainers = ["frankwillmore"]

    version("main", branch="main")

    depends_on("hdf5")
    depends_on("mpi")

    variant("plugins", default=False, description="Build plugins to generate other data flavors")

    def cmake_args(self):
        return [
            self.define_from_variant("BUILD_PLUGINS", "plugins")
        ]
