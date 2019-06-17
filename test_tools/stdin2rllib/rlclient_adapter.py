#!/usr/bin/env python3.6

import json
import sys
import rl_client

class my_error_callback(rl_client.error_callback):
    def on_error(self, error_code, error_message):
        print("Background error:")
        print(error_message)

def main():
    print("load rl config")
    json_str  = str(sys.argv[1])
    config = rl_client.create_config_from_json(json_str)

    print("init rl client")
    test_cb = my_error_callback()
    model = rl_client.live_model(config, test_cb)
    model.init()

    print("process stdin and send events")
    for line in sys.stdin:
        j = json.loads(line)
        if 'RewardValue' in j:
            print("report_outcome", j['EventId'], j['RewardValue'])
            model.report_outcome(j['EventId'], j['RewardValue'])
        else:
            print("choose_rank", j['EventId'])
            model.choose_rank(j['c'], j['EventId'])
            if j['_label_cost'] != 0:
                print("report_outcome", j['EventId'], -j['_label_cost'])
                model.report_outcome(j['EventId'], -j['_label_cost'])

    print("all events sent")

if __name__ == "__main__":
   main()
