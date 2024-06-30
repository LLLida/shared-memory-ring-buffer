# Реализация циклического буффера (ring buffer) для общения между независимыми процессами

Данная реализация использует API Posix для выделения shared memory и
использования её как кольцевого буффера. Использован трюк с маппингом
двух разных участков виртуальной памяти к одному для эффективной
реализации ring buffer.

## Детали реализации

Код написан на языке C11 и предназначен только для GNU/Linux.

Процесс writer создаёт ring buffer, вызывася *shm_open*, *ftruncate* и
*mmap*. Процессу, читающему из буффера, нужно только замапить память.

Кольцевой буффер в данном репозитории способен записывать и считывать
непрерывные куски памяти произвольного размера. При этом операции
являются быстрыми и не включают операций взятия по модулю. Это
достигается умным использованием механизма виртуальной памяти. Для
одного блока физической памяти выделяются два блока виртуальной
памяти, которые лежат друг за другом. Таким образом, имеем быстрый
кольцевой с почти 0 сложной логики внутри. Всю работу за нас делает
*Memory Management Unit* в железе. Больше информации в
[статье](https://lo.calho.st/posts/black-magic-buffer/).

Поскольку каждый процесс имеет свое адресное пространство, то
указатель на начало буффера каждый процесс имеет свой. Маппинг
виртуальных страниц для кольцевого буффера поэтому нужно проводить в
каждом отдельном процессе.

Из-за специфики задачи(всего лишь два процесса) я счёл достаточным
использовать простой lock на основе mutex-а и cond-а. Пишущий процесс
делает *pthread_cond_signal*, а читающий процесс ждет, вызывая
*pthread_cond_wait*.

Writer периодически пишет сообщения длиной 20-40 байт. Каждое
сообщение содержит в себе таймстемп и дополнительную ненужную для
задачи информацию.

Reader читает сообщения так быстро, как сможет и пишет информацию,
которая позже анализируется, в большой буффер. Буффер затем
записывается в файл, когда программа завершается. Это сделано, чтобы reader был максимально быстрым
и не тратил время на дополнительные IO операции.

Далее информация используется простенькими python скриптами, которые
выводят графики, показанные ниже.

## Запуск

Проект использует прекрасную систему сборки *make*.

Сборка бинарников(требуется *gcc* и стандартные Linux библиотеки):
```bash
make
```

Запуск бенчмарков(требуется *python3* и зависимости из файла *py/requirements.txt*):
```bash
make benchmark
```

## Анализ

С помощью python скрипта были перебраны различные конфигурации и
построены зависимости.

Измерения производились на ноутбке с 4ГБ оперативной памяти,
процессором Intel i3-8130U (4) @ 3.400GHz и операционной системой Arch
Linux.

### Маленький размер кольцевого буффера

Ниже приведена таблица графиков с измерениями. Были проведены тесты
для размеров буфферов 4Кб(размер страницы OS), 16Кб и 32Кб.

| 25 сообщений в секунду    |  50 сообщений в секунду    | 100 сообщений в секунду    | 200 сообщений в секунду     | 500 сообщений в секунду     | 750 сообщений в секунду     | 1000 сообщений в секунду     |
|---------------------------|----------------------------|----------------------------|-----------------------------|-----------------------------|-----------------------------|------------------------------|
| ![](img/graph_4K_25.png)  |  ![](img/graph_4K_50.png)  | ![](img/graph_4K_100.png)  |  ![](img/graph_4K_200.png)  |  ![](img/graph_4K_500.png)  |  ![](img/graph_4K_750.png)  |  ![](img/graph_4K_1000.png)  |
| ![](img/graph_16K_25.png) |  ![](img/graph_16K_50.png) | ![](img/graph_16K_100.png) |  ![](img/graph_16K_200.png) |  ![](img/graph_16K_500.png) |  ![](img/graph_16K_750.png) |  ![](img/graph_16K_1000.png) |
| ![](img/graph_32K_25.png) |  ![](img/graph_32K_50.png) | ![](img/graph_32K_100.png) |  ![](img/graph_32K_200.png) |  ![](img/graph_32K_500.png) |  ![](img/graph_32K_750.png) |  ![](img/graph_32K_1000.png) |

Время отклика варьируется от 60 мкс до 100 мкс. При увеличении
количества сообщений скорость обработки немного улучшается, но явной
зависимости не видно.

![](img/stats_small.png)

### Средний размер кольцевого буффера

Были проведены тесты для размеров буфферов 256Кб, 1Мб, 8Мб и 32Мб.

|  200 сообщений в секунду       | 500 сообщений в секунду        | 1000 сообщений в секунду        | 1500 сообщений в секунду        | 2000 сообщений в секунду        |
|--------------------------------|--------------------------------|---------------------------------|---------------------------------|---------------------------------|
|  ![](img/graph_256K_200.png)   |  ![](img/graph_256K_500.png)   |  ![](img/graph_256K_1000.png)   |  ![](img/graph_256K_1500.png)   |  ![](img/graph_256K_2000.png)   |
|  ![](img/graph_1024K_200.png)  |  ![](img/graph_1024K_500.png)  |  ![](img/graph_1024K_1000.png)  |  ![](img/graph_1024K_1500.png)  |  ![](img/graph_1024K_2000.png)  |
|  ![](img/graph_8192K_200.png)  |  ![](img/graph_8192K_500.png)  |  ![](img/graph_8192K_1000.png)  |  ![](img/graph_8192K_1500.png)  |  ![](img/graph_8192K_2000.png)  |
|  ![](img/graph_32768K_200.png) |  ![](img/graph_32768K_500.png) |  ![](img/graph_32768K_1000.png) |  ![](img/graph_32768K_1500.png) |  ![](img/graph_32768K_2000.png) |

Время ответа не меняется до 1000 сообщений в секунду. Зато он резко
падает при 1500 сообщений в секунду. При всех размерах буффера время
отклика при 1500 сообщений в секунду занимает около 50 мкс.

![](img/stats_big.png)

Вывод: при высоком частоте отправки сообщений время отклика падает.

## TODO

 - [ ] ring buffer без блокировок (не стал реализовывать, поскольку в задаче всего 2 потока);
 - [ ] протестировать разные размеры сообщений;
 - [ ] сделать CPU ID аргументом бинарников.
