import subprocess
import os
import signal
import time

from plot_results import plot_results

def run_benchmark(ring_buffer_size: int, frequency: int, outpath):
    print(f'--Benchmark: {ring_buffer_size=} {frequency=}')
    # launch the processes
    writer_process = subprocess.Popen(['./writer', str(ring_buffer_size), str(frequency)],
                                      stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(0.05)
    reader_process = subprocess.Popen(['./reader', str(ring_buffer_size), outpath],
                                      stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    time.sleep(20)
    # 20 seconds is enough for our statistics, stop execution of
    # processes
    os.kill(reader_process.pid, signal.SIGINT)
    os.kill(writer_process.pid, signal.SIGINT)
    reader_process.wait()

    # plot the resulting image
    imagepath = f'img/graph_{ring_buffer_size//1024}K_{frequency}.png'
    return plot_results(outpath, imagepath, f'{ring_buffer_size=} {frequency=}')

# buff_sizes = [4096, 4*4096, 1024*1024, 32*1024*1024]
buff_sizes = [1024*1024, 32*1024*1024]
freqs = [10, 50, 100, 200, 500, 750, 1000]

stats = []
for buff_size in buff_sizes:
    for freq in freqs:
        mu, _ = run_benchmark(buff_size, freq, 'temp.csv')
        print(f'\t{mu=}')
        stats.append((buff_size, freq, mu))

print(stats)
