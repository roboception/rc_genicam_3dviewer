1.2.0 (2020-11-27)
------------------

* Added saving of point cloud in PLY format into the user home directory with 'c'

1.1.1 (2020-11-25)
------------------

* Fixed possible segmentation fault on exit

1.1.0 (2019-12-04)
------------------

* Added simple dialog for choosing sensor if more than one is available

1.0.7 (2019-10-07)
------------------

* Implemented showing framerate with key 'i'

1.0.6 (2019-08-20)
------------------

* Ensure that depth acquisition is set to continuous

1.0.5 (2019-07-29)
------------------

- Fixes for compiling under Windows with new rc_genicam_api
- Added build instructions for Windows in README

1.0.4 (2019-06-19)
------------------

- Request receiving images individually (i.e. not synchronized) to avoid not getting
  buffers in alternate exposure active mode
- Improved readme

1.0.3 (2019-01-09)
------------------

- Fixed prefix of capture image file
- Improved readme

1.0.2 (2019-01-08)
------------------

- Included short hint about key codes in help text of tool
- Fixed synchronization bug when switching between exposure alternate and other modes
- Some fixes for compiling in Visual Studio under Windows

1.0.1 (2018-12-21)
------------------

- Only get images without projection in exposure alternate mode

1.0.0 (2018-12-21)
------------------

- Initial release
