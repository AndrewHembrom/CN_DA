import socket
import struct
import matplotlib.pyplot as plt
import math
import numpy as np
from scipy.signal import find_peaks
from scipy.signal import butter, filtfilt
from scipy.fft import fft, fftfreq
from scipy.ndimage import gaussian_filter1d
from pymongo import MongoClient
from os.path import exists
from scipy.io.wavfile import write
import pyaudio

# uri = "mongodb+srv://Local_Trial_Client:dvTxiBTJZAf1ZDcH@clusterlearning.uebi9hb.mongodb.net/?retryWrites=true&w=majority"
# client = MongoClient(uri)
# db = client.get_database("Trial_1")
# rec = db.BPM

# uri = "mongodb+srv://abel:65r7PNLqDF8gHUQ0@testing.iylygdv.mongodb.net/?retryWrites=true&w=majority"
# client = MongoClient(uri)
# dp = client.get_database("One")
# rec = dp.Beats

PatientID = 4817

audio = pyaudio.PyAudio()
stream = audio.open(format=pyaudio.paFloat32, channels=1, rate=8000, output=True, frames_per_buffer=4096)

peakY = []
peakX = []

def butter_lowpass_filter(sig, cutoff, order, Fs):
    # Get the filter coefficients 
    b, a = butter(order, Wn = cutoff, fs = Fs, btype='low', analog=False)
    y = filtfilt(b, a, sig)
    return y

def butter_highpass_filter(sig, cutoff, order, Fs):
    # Get the filter coefficients 
    b, a = butter(order, Wn = cutoff, fs = Fs, btype='highpass', analog=False)
    y = filtfilt(b, a, sig)
    return y

UDP_IP = "172.20.10.3"
UDP_PORT = 11211

sock = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP

sock.bind((UDP_IP, UDP_PORT))

startRecording = input("Start Recording (y/n)?")

if(startRecording == 'y'):

#----------------DATA ACQUISITION---------------
    startRecording = 'n'
    while(startRecording == 'n'):
        print("This is the audio that the mic is hearing:")

        for i in range(20):
            outData = []
            txData = bytes()

            data, addr = sock.recvfrom(131072) # buffer size is 4096 bytes
            # 131072
            y = []
            for i in range(0, 16384,4):
                #16384
                a = float(struct.unpack('<f', data[i: i+4])[0])
                #data[i : i+4]
                y.append(a)

            for i in y[:2048]:
                #2048
                outData.append(i)

            for i in outData:
                txData += bytes(struct.pack('<f', i))

            stream.write(txData)
        startRecording = input("Start Recording (y/n)?")

    outData = []
    print("Recording in progress")

    for i in range(60):

        print(str(i) + " out of 60")

        data, addr = sock.recvfrom(131072) # buffer size is 4096 bytes

        y_temp = []
        for i in range(0, 16384, 4):
            a = float(struct.unpack('<f', data[i : i+4])[0])
            y_temp.append(a)

        for i in y_temp[:2048]:
            outData.append(i)

#----------------DATA PROCESSING--------------

    y = outData # Should be recorded data
    x = [i for i in range(len(y))]
    N = len(y)
    fs = 8000.0
    T = 1.0/8000.0 # Time period of recording

    yf_prev = fft(y)
    xf_prev = fftfreq(N, T)[:N//2]

    lpf_y = butter_lowpass_filter(y, 110, 7, 8000)
    bpf_y = butter_highpass_filter(lpf_y, 30, 7, 8000)

    bpf_y = [10*i for i in bpf_y]
    bpf_y = np.array(bpf_y)

    yf = fft(bpf_y)
    xf = fftfreq(N, T)[:N//2]

    minY = min(bpf_y)
    posY = [(i + (minY * -1) + 1.0) for i in bpf_y]

    log_y = [math.log(i) for i in posY]

    minY = min(log_y)
    negY = [(i - minY) for i in bpf_y]
    # negY = [(i - minY) for i in log_y]

    lpf_log_y = butter_lowpass_filter(negY, 50, 7, 8000)
    # lpf_log_y = butter_lowpass_filter(log_y, 50, 7, 8000)

    e_lpf_log_y = [math.exp(i) for i in lpf_log_y]

    yf_after = fft(e_lpf_log_y)
    xf_after = fftfreq(N, T)[:N//2]

    meanY = sum(e_lpf_log_y)/len(e_lpf_log_y)
    medianY = sorted(e_lpf_log_y)[(len(e_lpf_log_y) - 1)//2]
    normalized_data = [(i - medianY)/meanY for i in e_lpf_log_y]

    yf_norm = fft(normalized_data)
    xf_norm = fftfreq(N, T)[:N//2]

    Gaussian_signal = gaussian_filter1d(normalized_data, 500)

    meanY = sum(Gaussian_signal)/len(Gaussian_signal)
    medianY = sorted(Gaussian_signal)[(len(Gaussian_signal) - 1)//2]
    Gaussian_signal = [(i - medianY)/meanY for i in Gaussian_signal]

    yf_gauss = fft(Gaussian_signal)
    xf_gauss = fftfreq(N, T)[:N//2]

    # threshold_height = sorted(Gaussian_signal)[math.ceil((len(Gaussian_signal)-1)/2)]
    threshold_height = sum(Gaussian_signal)/len(Gaussian_signal)
    peaks, _ = find_peaks(Gaussian_signal)

    peakPltY = []
    peakPltX = []
    for i in range(len(peaks)):
        peakPltY.append(Gaussian_signal[peaks[i]])
        peakPltX.append(x[peaks[i]])

    BPM = (len(peaks)/(2.0*len(Gaussian_signal)/8000.0))*60.0

    print("BPM: ", BPM)
    print("len(peaks): ", len(peaks))

    print("Sending the BPM data to the cloud...")

    # if(rec.find_one({"ID":PatientID}) == None):
    #     print("Record doesn't exist, creating a record")
    #     rec.insert_one({
    #         "ID":PatientID,
    #         "BPM":0
    #     })
    # else:
    #     print("Record found, Updating")

    # rec.update_one({"ID":PatientID}, {"$set": {"BPM" : BPM}})

    #----------------SAVE THE INITIAL CSV-----------------

    file_name = "audioData_BackendGG_"
    counter = 0
    while(exists(file_name + str(counter) + ".csv")):
        counter = counter + 1
    file = open (file_name + str(counter) + ".csv", 'w')
    for i in outData:
        file.write(str(i) + "\n")
    file.close()

    #------------------SAVE THE AUDIO----------------------

    file_name = "Backend_AudioGG_"
    counter = 0
    while(exists(file_name + str(counter) + ".wav")):
        counter = counter + 1
    write(file_name + str(counter)+ ".wav", 8000, bpf_y.astype(np.float32))

    #----------------PLOT THE RESULTS-----------------

    fig = plt.figure()
    fig.suptitle("Peaks, Heartrate = %f BPM" % BPM, fontsize=15)
    plt.plot(x, Gaussian_signal)
    plt.plot(peakPltX, peakPltY,'x')
    plt.plot([threshold_height for i in range(len(x))], linestyle = '--')

    rows = 3
    columns = 1

    fig1 = plt.figure()
    fig1.suptitle("Time Domain Signals", fontsize=15)
    ax1 = fig1.add_subplot(rows,columns,1)
    ax2 = fig1.add_subplot(rows,columns,2)
    ax3 = fig1.add_subplot(rows,columns,3)

    ax1.title.set_text("After BPF")
    ax2.title.set_text("After Normalization")
    ax3.title.set_text("After Gaussian")

    ax1.plot(x, bpf_y)
    ax2.plot(x, normalized_data)
    ax3.plot(x, Gaussian_signal)

    fig2 = plt.figure()

    plt.plot(xf_prev, 2.0/N * np.abs(yf_prev[0:N//2]), label = 'Before BPF')
    plt.plot(xf, 2.0/N * np.abs(yf[0:N//2]), label = 'After BPF')
    plt.plot(xf_norm, 2.0/N * np.abs(yf_norm[0:N//2]), label = 'After Normalization')
    plt.plot(xf_gauss, 2.0/N * np.abs(yf_gauss[0:N//2]), label = 'After Gaussian')

    plt.legend()
    plt.grid()
    plt.show()