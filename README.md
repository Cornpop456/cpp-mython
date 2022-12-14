# Mython

`Mython` - интерпретатор Python-подобного программирования.
Язык является динамически типизированным, поддерживает классы, наследование.

## Работа с интерпретатором
Подготовьте файл с корректным кодом на языке Mython. 
Запустите исполняемый файл Mython (Mython.exe в Windows) с двумя параметрами :
<имя входного файла с кодом> , <имя выходного файла для вывода результата>
Если программа синтаксически корректна - в выходной файл будет выведен результат работы программы.
Если в программе есть ошибки - в консоль будет выведена информация об ошибках.

## Синтаксис языка Mython
### Раздел в разработке...
(примеры программ на языке Mython можно найти в тестах в файле `parse_test.cpp`)

## Сборка и установка
Для сборки программы необходим компилятор С++ поддерживающий стандарт не ниже С++17

> ## Сборка
> 1. Скомпилируйте cpp файлы `g++ *.cpp -o mython"

## Использование интерпретатора

1. Подготовьте в папке с интерпретатором Mython файл с исходным кодом на языке Mython (например `"test.my"`)
<details>
  <summary>Пример корректного файла test.my - программа вычисляющая НОД(наибольший общий делитель)</summary>

 ```
class GCD:
  def __init__():
    self.call_count = 0

  def calc(a, b):
    self.call_count = self.call_count + 1
    if a < b:
      return self.calc(b, a)
    if b == 0:
      return a
    return self.calc(a - b, b)

x = GCD()
print x.calc(510510, 18629977)
print x.calc(22, 17)
print x.calc(100, 30)
print x.call_count
```
</details>


2. Запустите интерпретатор Mython - в качестве первого аргумента - файл с программой на исполнение, в качестве второго - файл для вывода результата :
```
./Mython test.my out.txt
``` 

3. В папке создатся файл `out.txt` в котором будет результат работы программы. 
<details>
  <summary>Пример вывода в файл `out.txt` для программы выше:</summary>

  ```
17
1
10
124
  ```
</details>
