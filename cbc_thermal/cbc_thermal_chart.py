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
import time
import re
import xml.etree.ElementTree as etree

thermal_sensors = []
thermal_cdevs = []

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
    for i in range(10):
        with open("/sys/class/thermal/thermal_zone{0}/type".format(i)) as f:
                s = {}
                s["type"] = f_type.read().rstrip()
                s["path"] = "/sys/class/thermal/thermal_zone{0}/temp".format(i)
                thermal_sensors.append(s)

def scan_sensors():
    for i in range(99):
        try:
            with open("/sys/class/thermal/thermal_zone{0}/type".format(i)) as f:
                s = {}
                s["type"] = f.read().rstrip()
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
                c["path"] = "/sys/class/thermal/cooling_device{0}/cur_state".format(i)
                tmp = open(c["path"]).read()
                c["max"] = open("/sys/class/thermal/cooling_device{0}/max_state".format(i)).read().rstrip()
                thermal_cdevs.append(c)
        except:
            pass

def gen_svg_header(width, height):
    return """
<?xml version="1.0" standalone="no"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg width="
""" + width + """
px" height="
""" + height + """
px" version="1.1" xmlns="http://www.w3.org/2000/svg">
<defs>
  <style type="text/css">
    <![CDATA[
      rect       { stroke-width: 1; }
      rect.bg    { fill: rgb(255,255,255); }
      rect.cooling   { fill: rgb(64,64,240); stroke-width: 0; fill-opacity: 0.7; }
      rect.busy  { fill: rgb(240,240,0); stroke-width: 0; fill-opacity: 0.7; }
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

def gen_svg_end():
    return """</svg>"""

def gen_svg_comment(y, msg[]):
    body = '<g transform="translate(10,  {0})">'.format(y)
    for line in msg:
        y = y + 20
        body += '<text class="t1" x="0" y="{0}">{1}</text>'.format(y, line)
    body += """</g>"""
    y = y + 20
    return y, body

def gen_svg_graph(y, color, title, data[], max_val, min_val):
    y = y + 140
    body = '<g transform="translate(10,{0})">'.format(y)
    body += '<text class="t2" x="5" y="-15">{0}</text>'.format(title)
    scale = 100 / (max_val - min_val + 1)
    sec01_index = 0
    sec10_index = 0
    tmp = 0;
    for val in data:
        val = (val - min_val) * scale
        body += '<line class="sec01" x1="{0}" y1="0" x2="{0}" y2="100.000" />'.format(sec01_index * 20)
        body += '<rect class="{0}" x="{1}" y="{2}" width="19" height="{3}" />'.format(color, sec01_index * 20, 100 - val, val)
        tmp += 1
        if tmp == 10:
            tmp = 0
            sec10_index += 1
            body += '<text class="sec" x="{0}" y="-5.000" >{1}</text>'.format(sec01_index * 20, sec10_index)
        sec01_index += 1
    body += """</g>"""
    return y, body

def main(args):
    pp.pprint(args)
    scan_sensors()
    scan_cdevs(args.cpu_all)
    scan_thremald_config(args.config, args.product)
    pp.pprint(thermal_sensors)
    pp.pprint(thermal_cdevs)

    start_ts = time.time()
    with open("{0}/cbc_thermal_chart.csv".format(args.output_dir), 'w') as file_csv:
        line = "ts,"
        for col in thermal_sensors + thermal_cdevs:
            line += "{0},".format(col["type"])
        line = line[:-1] + "\n"
        file_csv.write(line)

        while time.time() - start_ts < args.time:
            line = "{0},".format(time.time())
            for col in thermal_sensors + thermal_cdevs:
                print("fetch: {0}".format(col["path"]))
                line += "{0},".format(open(col["path"]).read().rstrip())
            line = line[:-1] + "\n"
            file_csv.write(line)
            time.sleep(args.interval)

def parse_arguments(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--interval', type=int, default=1, help='Poll interval in seconds. [1]')
    parser.add_argument('--output-dir', type=str, default="/run/log/", help='Path to output files. [/run/log/]')
    parser.add_argument('--product', type=str, default="*", help='product name. [*]')
    parser.add_argument('--config', type=str, default="/usr/share/defaults/cbc_thermal/thermal-conf.xml", help='CBC Thermald config. [/usr/share/defaults/cbc_thermal/thermal-conf.xml]')
    parser.add_argument('--time', type=int, default=300, help='Record time in seconds. [300]')
    parser.add_argument('--cpu-all', action='store_const', const=True, help='Poll all cpu status, default: cpu0 only')
    return parser.parse_args(argv)

if __name__ == '__main__':
    pp = pprint.PrettyPrinter(indent=4)
    args = parse_arguments(sys.argv[1:])
    main(args)
