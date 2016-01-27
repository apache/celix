#!/usr/bin/python
""" Setup.py for Cog
    http://nedbatchelder.com/code/cog

    Copyright 2004-2015, Ned Batchelder.
"""

from distutils.core import setup

setup(
    name = 'celix-bootstrap',    
    version = '0.1.0',
    url = '',
    author = '',
    author_email = '',
    description =
        'celix-bootstrap: A code generator / project generator for Apache Celix.',

    license = 'Apache License, Version 2.0',

    packages = [
        'celix',
	'celix/bootstrap'
        ],

    scripts = [
        'scripts/celix-bootstrap',
        ],

    package_data={'celix' : ['bootstrap/templates/**/*']},
)
