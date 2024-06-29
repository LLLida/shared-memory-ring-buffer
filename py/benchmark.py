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

    # plot the resulting image
    imagepath = f'img/graph_{ring_buffer_size}_{frequency}.png'
    plot_results(outpath, imagepath, f'{ring_buffer_size=} {frequency=}')

run_benchmark(4096, 10, 'temp.csv')
