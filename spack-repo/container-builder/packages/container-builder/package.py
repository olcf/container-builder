from spack import *
import os

class ContainerBuilder(CMakePackage):
    """container-builder Client"""

    homepage = "https://code.ornl.gov/olcf/container-builder.git"
    url      = "https://code.ornl.gov/olcf/container-builder/archive/master.zip"

    version('master', git="https://code.ornl.gov/olcf/container-builder.git")
    ci_tag = os.environ.get('CI_COMMIT_TAG')
    if ci_tag:
        version(ci_tag, git="https://code.ornl.gov/olcf/container-builder.git", tag=ci_tag)

    depends_on('boost@1.66.0')
