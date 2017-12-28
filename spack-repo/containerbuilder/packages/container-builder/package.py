from spack import *
import os

class ContainerBuilder(CMakePackage):
    """ContainerBuilder Client"""

    homepage = "https://github.com/AdamSimpson/ContainerBuilder"
    url      = "https://github.com/AdamSimpson/ContainerBuilder/archive/master.zip"

    version('master', git="https://code.ornl.gov/olcf/ContainerBuilder.git")
    ci_tag = os.environ.get('CI_COMMIT_TAG')
    if ci_tag:
        version(ci_tag, git="https://code.ornl.gov/olcf/ContainerBuilder.git", tag=ci_tag)

    depends_on('boost@1.66.0+coroutine')
