import subprocess
import os
import signal
import time
import matplotlib.pyplot as plt

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

def plot_stats(stats, name):
    measurements = {}
    for size, x, y1, y2 in stats:
        if size in measurements:
            measurements[size][0].append(x)
            measurements[size][1].append(y1)
            measurements[size][2].append(y2)
        else:
            measurements[size] = ([], [], [])

    colors = ['#cd853f', '#7cfc00', '#00bfff', '#9400d3', '#a52a2a', '#cd5c5c', '#00ff7f', 'orange', '#bdb76b', 'yellow']
    i = 0
    fig, ax = plt.subplots(figsize=(6, 4))

    for key, (x, y1, y2) in measurements.items():
        ax.plot(x, y1, label=f'mean {key//1024}KB', color=colors[i], linestyle='--')
        ax.plot(x, y2, label=f'p99 {key//1024}KB', color=colors[i])
        ax.scatter(x, y2, color='black')
        i += 1
    ax.set_title(f'Respond time depending on frequency and size of ring buffer')
    ax.legend()
    ax.grid()
    ax.set_ylabel('respond duration, seconds')
    ax.set_xlabel('messages per second')
    plt.savefig(name)
    print(f'Saved plot to {name}...')
    plt.close()


def run_benchmarks(buff_sizes, freqs):
    stats = []
    for buff_size in buff_sizes:
        for freq in freqs:
            mu, p99 = run_benchmark(buff_size, freq, 'temp.csv')
            print(f'\t{mu=} {p99=}')
            stats.append((buff_size, freq, mu, p99))
    return stats

buff_sizes = [4096, 4*4096, 8*4096]
freqs = [25, 50, 100, 200, 500, 750, 1000]
plot_stats(run_benchmarks(buff_sizes, freqs), 'img/stats_small.png')

buff_sizes = [256*1024, 1024*1024, 8*1024*1024, 32*1024*1024]
freqs = [200, 500, 750, 1000, 1500, 2000]
plot_stats(run_benchmarks(buff_sizes, freqs), 'img/stats_big.png')
