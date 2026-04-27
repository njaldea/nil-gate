from setuptools import setup, find_packages
from setuptools.dist import Distribution


class BinaryDistribution(Distribution):
    """Distribution class for binary wheel packages."""
    def has_ext_modules(self):
        return True


if __name__ == "__main__":
    setup(
        packages=find_packages(include=["nil_gate*"]),
        distclass=BinaryDistribution,
    )
