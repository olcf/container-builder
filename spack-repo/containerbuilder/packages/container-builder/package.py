from spack import *

class ContainerBuilder(CMakePackage):
    """ContainerBuilder Client"""

    homepage = "https://github.com/AdamSimpson/ContainerBuilder"
    url      = "https://github.com/AdamSimpson/ContainerBuilder/archive/master.zip"

    version('0.0.0')

    depends_on('boost@1.65.1')
