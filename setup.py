from setuptools import find_packages, setup


setup(
    name='signature-recognition',
    packages=find_packages(),
    version='0.0.1',
    install_requires=[
        'svgwrite',
        'svgpathtools==1.2.3',
        'numpy',
        'click'
    ],
    entry_points={
        'console_scripts': [
            'signature-recognition = signature_recognition.cli:tool'
        ]
    }
)
