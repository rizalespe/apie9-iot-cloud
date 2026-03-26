import json
import boto3
import time

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('ESP32SensorData')

def lambda_handler(event, context):
    print(f"Event received: {json.dumps(event)}")
    
    try:
        device_id   = event.get('device_id', 'unknown')
        distance_cm = int(event.get('distance_cm', 0))
        detected    = int(event.get('detected', 0))
        msg_count   = int(event.get('msg_count', 0))
        uptime_ms   = int(event.get('uptime_ms', 0))
        timestamp   = int(event.get('timestamp', time.time() * 1000))
        status      = "OBJECT DETECTED" if detected == 1 else "CLEAR"

        item = {
            'device_id'   : device_id,
            'timestamp'   : timestamp,
            'distance_cm' : distance_cm,
            'detected'    : detected,
            'status'      : status,
            'msg_count'   : msg_count,
            'uptime_ms'   : uptime_ms
        }

        table.put_item(Item=item)
        print(f"Saved: {status} | distance: {distance_cm} cm | device: {device_id}")

        return {
            'statusCode': 200,
            'body': json.dumps(f'{status} - {distance_cm} cm saved successfully')
        }

    except Exception as e:
        print(f"Error: {str(e)}")
        return {
            'statusCode': 500,
            'body': json.dumps(f'Error: {str(e)}')
        }