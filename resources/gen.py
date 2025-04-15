#!/bin/python3
import random
import csv
import argparse

names = [ "Alice Johnson", "Bob Smith", "Carmen Ruiz", "David Lee", "Emma Davis", "Frank Miller", "Grace Kim", "Henry Wilson", "Isabella Rossi", "Jack Thompson" ]
ages = [ "28", "35", "22", "30", "42", "29", "33", "26", "31", "37" ]
emails = [ "alice.johnson@example.com", "bob.smith@example.com", "carmen.ruiz@example.com", "david.lee@example.com", "emma.davis@example.com", "frank.miller@example.com", "grace.kim@example.com", "henry.wilson@example.com", "isabella.rossi@example.com ", "jack.thompson@example.com" ]
cities = [ "New York", "London", "Madrid", "Sydney", "Toronto", "Berlin", "Seoul", "Dublin", "Rome", "Auckland" ]
phones = [ "555-1234", "020-7946-0018", "91-2345-678", "02-9876-5432", "416-555-0123", "030-12345678", "02-555-7890", "01-87654321", "06-4321-9876", "09-123-4567" ]

parser = argparse.ArgumentParser()
parser.add_argument("--rows", type=int, default=1000)
parser.add_argument("--output", type=str, default="output.csv")
args = parser.parse_args()

with open(args.output, mode='w', newline='') as file:
    lines = ''
    for cur in range(args.rows):
        lines += random.choice(names) + "," + random.choice(ages) + "," + random.choice(emails) + "," +  random.choice(cities) + "," + random.choice(phones) + "\n"
        if cur % 4096:
            file.write(lines)
            lines = ''
    if len(lines) > 0:
        file.write(lines)
