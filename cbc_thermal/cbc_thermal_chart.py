#!/usr/bin/env python3
# vim:expandtab:autoindent:tabstop=4:shiftwidth=4:filetype=python:textwidth=0:
# -*- coding: utf-8 -*-
#
#Copyright (C) 2018 Intel Corporation
#Author: Yang Bin<bin.yang@intel.com>
#SPDX-License-identifier: BSD-3-Clause

import pprint
import sys
import os
import argparse
import datetime
import time
import re
import xml.etree.ElementTree as etree
import threading
import mmap

thermal_sensors = []
thermal_cdevs = []
thermal_rapls_pl = []
thermal_rapls_uw = []
thermal_rapl_max_val = 0

def scan_thremald_config(config, product):
    global thermal_sensors
    global thermal_cdevs

    try:
        tree = etree.parse(config)
        root = tree.getroot()
        plt = root.find("./Platform[ProductName='{0}']".format(product));
        for sensor in plt.find("./ThermalSensors"):
            s = {}
            s["type"] = sensor.find("Type").text
            s["path"] = sensor.find("Path").text
            thermal_sensors.append(s)
        for cdev in plt.find("./CoolingDevices"):
            c = {}
            c["type"] = cdev.find("Type").text
            c["path"] = cdev.find("Path").text
            c["max"] = cdev.find("MaxState").text
            thermal_cdevs.append(c)
    except:
        print("parse {0} error".format(config))

def scan_sensors():
    for i in range(99):
        try:
            with open("/sys/class/thermal/thermal_zone{0}/type".format(i)) as f:
                s = {}
                s["type"] = f.read().rstrip() + ":z{0}".format(i)
                s["path"] = "/sys/class/thermal/thermal_zone{0}/temp".format(i)
                tmp = open(s["path"]).read()
                thermal_sensors.append(s)
        except:
            pass

def scan_cdevs(all_cpu):
    cpu_index = 0
    for i in range(99):
        try:
            with open("/sys/class/thermal/cooling_device{0}/type".format(i)) as f:
                c = {}
                c["type"] = f.read().rstrip()
                if c["type"] == "Processor":
                    if (cpu_index > 0 and not all_cpu):
                        continue
                    c["type"] = "cdev_cpu_{0}".format(cpu_index)
                    cpu_index = cpu_index + 1
                else:
                    c["type"] += ":c{0}".format(i)
                c["path"] = "/sys/class/thermal/cooling_device{0}/cur_state".format(i)
                tmp = open(c["path"]).read()
                c["max"] = open("/sys/class/thermal/cooling_device{0}/max_state".format(i)).read().rstrip()
                thermal_cdevs.append(c)
        except:
            pass

def scan_rapl():
    global thermal_rapl_max_val
    rapl_dirs = os.listdir("/sys/class/powercap")
    rapl_dirs.sort()
    for rd in rapl_dirs[1:]:
        try:
            with open("/sys/class/powercap/{0}/name".format(rd)) as f_name:
                name = f_name.read().rstrip()
                uw = {}
                uw["ts"] = time.time()
                uw["type"] = "{0}:{1}:energy_uw".format(rd, name)
                uw["path"] = "/sys/class/powercap/{0}/energy_uj".format(rd)
                uw["uj"] = int(open(uw["path"]).read().rstrip())
                thermal_rapls_uw.append(uw)
                try:
                    for i in range(99):
                        with open("/sys/class/powercap/{0}/constraint_{1}_name".format(rd, i)) as f_c_name:
                            pl = {}
                            pl["type"] = "{0}:{1}:{2}".format(rd, name, f_c_name.read().rstrip())
                            pl["path"] = "/sys/class/powercap/{0}/constraint_{1}_power_limit_uw".format(rd, i)
                            tmp = open(pl["path"]).read()
                            max_val = int(open("/sys/class/powercap/{0}/constraint_{1}_max_power_uw".format(rd, i)).read().rstrip())
                            if max_val > thermal_rapl_max_val:
                                thermal_rapl_max_val = max_val
                            thermal_rapls_pl.append(pl)
                except Exception as ex:
                    pass
        except Exception as ex:
            pass

def gen_svg_header(width, height):
    header0 = """<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
"""
    header1 = """
<defs>
  <style type="text/css">
    <![CDATA[
      rect       { stroke-width: 1; }
      rect.box   { fill: rgb(240,240,240); stroke: rgb(192,192,192); }
      rect.bg    { fill: rgb(255,255,255); }
      rect.cooling   { fill: rgb(64,200,64); stroke-width: 0; fill-opacity: 0.7; }
      rect.limit   { fill: rgb(255,255,0); stroke-width: 0; fill-opacity: 0.7; }
      rect.busy  { fill: rgb(64,64,240); stroke-width: 0; fill-opacity: 0.7; }
      rect.temp    { fill: rgb(192,64,64); stroke-width: 0; fill-opacity: 0.7; }
      line       { stroke: rgb(64,64,64); stroke-width: 1; }
      line.sec10  { stroke-width: 2; }
      line.sec01 { stroke: rgb(224,224,224); stroke-width: 1; }
      line.dot   { stroke-dasharray: 2 4; }
      line.idle  { stroke: rgb(64,64,64); stroke-dasharray: 10 6; stroke-opacity: 0.7; }
      text       { font-family: Verdana, Helvetica; font-size: 10; }
      text.sec   { font-size: 8; }
      text.t1    { font-size: 24; }
      text.t2    { font-size: 12; }
    ]]>
   </style>
</defs>

<rect class="bg" width="100%" height="100%" />
"""
    return '{0} <svg width="{2}px" height="{3}px" version="1.1" xmlns="http://www.w3.org/2000/svg"> {1}'.format(header0, header1, width, height)

def gen_svg_end():
    return """</svg>"""

def gen_svg_comment(y, msg):
    body = '<g transform="translate(10,  {0})">\n'.format(y)
    for line in msg:
        y = y + 20
        body += '<text class="t1" x="0" y="{0}">{1}</text>\n'.format(y, line)
    body += '</g>\n\n'
    y = y + 60
    return y, body

def gen_svg_graph(y, color, title, data, max_val, min_val):
    body = '<g transform="translate(10,{0})">\n'.format(y)
    body += '<rect class="box" x="0.000" y="0" width="{0}" height="100.000" />\n'.format(len(data) * 20)
    body += '<text class="t2" x="8" y="-20">{0}</text>\n'.format(title)
    body += '<text class="t3" x="4" y="20">{0}</text>\n'.format(max_val)
    body += '<text class="t3" x="4" y="100">{0}</text>\n'.format(min_val)
    scale = 100 / (max_val - min_val + 0.01)
    sec01_index = 0
    sec10_index = 0
    tmp = 0;
    body += '<line class="sec10" x1="{0}" y1="0" x2="{0}" y2="100.000"/><text class="sec" x="{0}" y="-5.000" >{1}</text>\n'.format(0, 0)
    for val in data:
#        print("val: {0}".format(val))
        val = float(val)
        if val < min_val:
            val = min_val
        if val > max_val:
            val = max_val
        val = (val - min_val) * scale
        body += '<line class="sec01" x1="{0}" y1="0" x2="{0}" y2="100.000" />\n'.format(sec01_index * 20)
        body += '<rect class="{0}" x="{1}" y="{2}" width="19" height="{3}" />\n'.format(color, sec01_index * 20, 100 - val, val)
        tmp += 1
        if tmp == 10:
            tmp = 0
            sec10_index += 1
            body += '<line class="sec10" x1="{0}" y1="0" x2="{0}" y2="100.000"/><text class="sec" x="{0}" y="-5.000" >{1}</text>\n'.format(sec01_index * 20, sec10_index * 10)
        sec01_index += 1
    body += '</g>\n\n'
    y = y + 160
    return y, body

class intel_gpu_usage_sampling(threading.Thread):
    lock = threading.Lock()
    gpu_usage = 0
    pcimm = None
    total_samples = 0
    idle_samples = 0
    stop_pending = 0
    def __init__(self):
        threading.Thread.__init__(self)
        try:
            with open("/sys/bus/pci/devices/0000:00:02.0/vendor", "r") as fd_vendor:
                print("open vendor success")
                if fd_vendor.read().rstrip() == "0x8086":
                    print("check vendor success")
                    with open("/sys/bus/pci/devices/0000:00:02.0/class", "r") as fd_class:
                        if (int(fd_class.read().rstrip(), 16) & 0x30000) == 0x30000:
                            with open("/sys/bus/pci/devices/0000:00:02.0/resource0", "rb") as fd:
                                self.pcimm = mmap.mmap(fd.fileno(), 0, prot=mmap.PROT_READ)
        except:
            pass
        if not self.pcimm:
            print("cannot find intel gpu device")
    def is_valid(self):
        return not self.pcimm == None
    def stop(self):
#        print("del gpu sampling ...")
        if self.is_valid():
            self.stop_pending = True
            self.join()
    def get_usage(self):
        self.lock.acquire()
        usage = self.gpu_usage
        self.lock.release()
        return usage
    def run(self):
        while not self.stop_pending:
            ring_head = int.from_bytes(self.pcimm[0x2034:0x2038], byteorder='little') & 0x001FFFFC
            ring_tail = int.from_bytes(self.pcimm[0x2030:0x2034], byteorder='little') & 0x000FFFF8
            self.total_samples += 1
            if ring_head == ring_tail:
                self.idle_samples += 1
            if self.total_samples == 100:
                self.lock.acquire()
                self.gpu_usage = 100 - self.idle_samples * 100 / self.total_samples
                if self.gpu_usage < 0.01:
                    self.gpu_usage = 0
                self.idle_samples = self.total_samples = 0
                self.lock.release()
            time.sleep(0.01)

cpu_total_save = 0
cpu_idle_save = 0
def get_cpu_usage():
    global cpu_total_save
    global cpu_idle_save
    cpu_usage = 0

    with open("/proc/stat") as file_stat:
        cpu_stat = file_stat.readline().rstrip().split()
        del cpu_stat[0]
        cpu_idle = int(cpu_stat[3])
        cpu_total = 0
        for tmp in cpu_stat:
            cpu_total += int(tmp)
        if cpu_total > cpu_total_save:
            if not cpu_total_save == 0:
                total_diff = cpu_total - cpu_total_save
                idle_diff = cpu_idle - cpu_idle_save
                cpu_usage = (1000 * (total_diff - idle_diff) / total_diff) / 10
                if cpu_usage < 0.01:
                    cpu_usage = 0.01
            cpu_total_save = cpu_total
            cpu_idle_save = cpu_idle
    return cpu_usage

def get_cpu_freq_range(cpu):
    cpu_max_freq = 0
    cpu_min_freq = 0
    with open("/sys/devices/system/cpu/cpu{0}/cpufreq/scaling_max_freq".format(cpu)) as file_freq:
        cpu_max_freq = int(file_freq.read().rstrip())
    with open("/sys/devices/system/cpu/cpu{0}/cpufreq/scaling_min_freq".format(cpu)) as file_freq:
        cpu_min_freq = int(file_freq.read().rstrip())
    return cpu_min_freq,cpu_max_freq

def get_cpu_freq(cpu):
    cpu_freq = 0
    try:
        with open("/sys/devices/system/cpu/cpu{0}/cpufreq/scaling_cur_freq".format(cpu)) as file_freq:
            cpu_freq = int(file_freq.read().rstrip())
    except:
        pass
    return cpu_freq

def record_to_csv(file_csv, duration, interval, all_cpu):
    get_cpu_usage()
    start_ts = time.time()

    line = "ts,cpu%,"

    cpu_num = 1
    if all_cpu:
        for i in range(1, 99):
            if get_cpu_freq(i) == 0:
                break;
            cpu_num += 1
    for i in range(cpu_num):
        line += "cpu{0}_freq,".format(i)

    gpu_sampling = intel_gpu_usage_sampling()
    if gpu_sampling.is_valid():
        gpu_sampling.start()
        line += "gpu%,"

    for col in thermal_rapls_uw + thermal_rapls_pl + thermal_sensors + thermal_cdevs:
        line += "{0},".format(col["type"])
    line = line[:-1] + "\n"
    file_csv.write(line)
    while time.time() - start_ts < duration:
        line = "{0},".format(time.time())

        cpu_usage = get_cpu_usage()
        line += "{0},".format(cpu_usage)

        for i in range(cpu_num):
            cpu_freq = get_cpu_freq(i)
            line += "{0},".format(cpu_freq)

        if gpu_sampling.is_valid():
            gpu_usage = gpu_sampling.get_usage()
            line += "{0},".format(gpu_usage)

        for uw in thermal_rapls_uw:
            ts = time.time()
            uj = int(open(uw["path"]).read().rstrip())
            uw_val = (uj - uw["uj"]) / (ts - uw["ts"])
            line += "{0},".format(uw_val)
            uw["uj"] = uj
            uw["ts"] = ts

        for col in thermal_rapls_pl + thermal_sensors + thermal_cdevs:
#            print("fetch: {0}".format(col["path"]))
            line += "{0},".format(open(col["path"]).read().rstrip().strip('\x00'))
        line = line[:-1] + "\n"
        file_csv.write(line)
#        print("{0}".format(line))
        time.sleep(interval)
    if gpu_sampling.is_valid():
        gpu_sampling.stop()
 
def csv_to_svg(file_csv, file_svg, comments):
    data_all = {}
    title_all = file_csv.readline().rstrip().split(',')
    for t in title_all:
        data_all[t] = []
    line = file_csv.readline().rstrip()
    while line:
        for key, data in zip(title_all, line.split(',')):
            data_all[key].append(data)
        line = file_csv.readline().rstrip()
#    pp.pprint(title_all)
#    pp.pprint(data_all)

    samples_num = len(data_all["ts"])
    titles_num = len(title_all)
    comments_num = len(comments)
    file_svg.write(gen_svg_header(samples_num * 20 + 200, titles_num * 160 + comments_num * 20))

    y,body = gen_svg_comment(0, comments)
    file_svg.write(body)

    re_is_cpufreq = re.compile(r"^cpu[0-9]+_freq$")
    re_get_num = re.compile(r"[0-9]+")
    for title in title_all:
        if title == "ts":
            continue
        color = "busy"
        max_val = 100
        if title in [x["type"] for x in thermal_sensors]:
            color = "temp"
            max_val = max(data_all[title])
            if float(max_val) > 1000:
                max_val = 110000
            else:
                max_val = 110
        elif title in [x["type"] for x in thermal_cdevs]:
            color = "cooling"
            max_val = dict(zip([x["type"] for x in thermal_cdevs], [x["max"] for x in thermal_cdevs]))[title]
        elif title in [x["type"] for x in thermal_rapls_uw]:
            color = "temp"
            max_val = thermal_rapl_max_val
        elif title in [x["type"] for x in thermal_rapls_pl]:
            color = "limit"
            max_val = thermal_rapl_max_val
        elif re_is_cpufreq.match(title):
            i = re_get_num.search(title).group(0)
            tmp,max_val = get_cpu_freq_range(i)
        y,body = gen_svg_graph(y, color, title, data_all[title], float(max_val), 0)
        file_svg.write(body)

    file_svg.write(gen_svg_end())

def main(args):
#    pp.pprint(args)
    date_now = datetime.datetime.now()
    date_now_str = "{0}{1}{2}-{3}{4}".format(date_now.year, date_now.month, date_now.day, date_now.hour, date_now.minute)
    scan_sensors()
    scan_cdevs(args.cpu_all)
    scan_rapl()
    scan_thremald_config(args.config, args.product)
   # pp.pprint(thermal_sensors)
   # pp.pprint(thermal_cdevs)
   # pp.pprint(thermal_rapls_pl)
   # pp.pprint(thermal_rapls_uw)
   # pp.pprint(thermal_rapl_max_val)

    sample_start_ts = time.time()
    with open("{0}/cbc_thermal_chart-{1}.csv".format(args.output_dir, date_now_str), 'w') as file_csv:
        record_to_csv(file_csv, args.time, args.interval, args.cpu_all)

    with open("{0}/cbc_thermal_chart-{1}.csv".format(args.output_dir, date_now_str), 'r') as file_csv:
        with open("{0}/cbc_thermal_chart-{1}.svg".format(args.output_dir, date_now_str), 'w') as file_svg:
            csv_to_svg(file_csv, file_svg, ["sample start: {0}".format(sample_start_ts), "sample interval: {0} sec".format(args.interval), "sample duration: {0} sec".format(args.time)])

def parse_arguments(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--interval', type=int, default=1, help='Poll interval in seconds. [1]')
    parser.add_argument('--output-dir', type=str, default="/run/log/", help='Path to output files. [/run/log/]')
    parser.add_argument('--product', type=str, default="*", help='product name. [*]')
    parser.add_argument('--config', type=str, default="/usr/share/ioc-cbc-tools/thermal-conf.xml", help='CBC Thermald config. [/usr/share/ioc-cbc-tools/thermal-conf.xml]')
    parser.add_argument('--time', type=int, default=300, help='Record time in seconds. [300]')
    parser.add_argument('--cpu-all', action='store_const', const=True, help='Poll all cpu status, default: cpu0 only')
    return parser.parse_args(argv)

if __name__ == '__main__':
    pp = pprint.PrettyPrinter(indent=4)
    args = parse_arguments(sys.argv[1:])
    main(args)
