# test_fmod
Test new fmod function.


| Test type (i5)  | dummy (spb)               | libm (spb)     | new (spb)   | ratio libm/new      |
| --------------- | ------------------------- | -------------- | ----------- | ------------------- |
| main            | 4.72                      | 1420.81        | 499.61      | 2.86                |
| 2\*pi wrap      | 8.82                      | 17.71          | 17.09       | 1.07                |
| fractional part | 7                         | 42.68          | 27.5        | 1.74                |
| extreme         | 4.93                      | 10394.9        | 570.71      | 18.36               |



| Test type (PI 4)| dummy (spb)               | libm (spb)     | new (spb)   | ratio libm/new      |
| --------------- | ------------------------- | -------------- | ----------- | ------------------- |
| main            | 11.95                     | 2085.13        | 234.31      | 9.32                |
| 2\*pi wrap      | 18.7                      | 31.5           | 27.25       | 1.50                |
| fractional part | 18.7                      | 76.17          | 33.06       | 4.00                |
| extreme         | 12.06                     | 13150.41       | 584.27      | 22.96               |


Performance comparison table
System GNU gcc 10 from Ubuntu 20.04 64bit:
* **i5**: Intel(R) Core(TM) i5-6300HQ
* **PI4** : Raspberry PI 4 4GB

Calculation functions:
* **dummy**: dummy function call for loop time itself
* **libm**: libm fmod function
* **new** : proposed new fmod function

Data:
* **spb**   : second per billion iterations (fmod calls). Lower better.
* **ratio** : (libm-dummy)/(new-dummy). Higher better new function performance.

Cases:
* **main**  : All range of exponential part in double, random mantissa and all range of CLZ and CTZ.
* **2\*pi wrap** : `fmod(x, 2 * M_PI)` x = [-4*pi,4*pi]
* **fractional part** : `fmod(x, 1.0)` x = [-1E6,1E6]
* **extreme** : `fmod(x, y)` where x ~ 1.1e+300, y ~ 1e-320 

