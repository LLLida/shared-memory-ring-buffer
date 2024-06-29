import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def plot_results(filepath, imagepath, title='Performance'):
    df = pd.read_csv(filepath)

    fig, ax = plt.subplots(figsize=(6, 4))

    send_sec = df['send_sec']
    send_sec -= send_sec.min()

    retrieve_sec = df['send_sec']
    retrieve_sec -= send_sec.min()

    send = send_sec + df['send_nsec']/(10**9)
    retrieve = retrieve_sec + df['retrieve_nsec']/(10**9)

    # skip first 5 seconds because measurements at start are unstable
    ind = np.searchsorted(send, 5.0)
    send = send[ind:]
    retrieve = retrieve[ind:]

    diff = np.clip(retrieve-send, 0, 1)
    mu = np.mean(diff)
    std = np.std(diff)

    ax.set_title(title+f'\nduration={mu*1000000:.2f}+-{std*1000000:.2f} microsec')
    ax.plot(send, retrieve-send)
    ax.grid()
    ax.set_xlabel('time, sec')
    ax.set_ylabel('duration, sec')
    ax.set_yscale('log')

    plt.savefig(imagepath)
    print(f'saved plot to {imagepath}...')
    plt.close()

    return mu, std

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('inpath', help='Path to results csv file')
    parser.add_argument('outpath', help='Path to output image')
    args = parser.parse_args()

    plot_results(args.inpath, args.outpath)
