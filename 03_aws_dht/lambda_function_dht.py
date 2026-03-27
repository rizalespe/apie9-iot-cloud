import json
import boto3
import time

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('ESP32SensorData')

def lambda_handler(event, context):
    print(f"Event received: {json.dumps(event)}")
    
    try:
        device_id               = event.get('device_id', 'unknown')
        humidity                = int(event.get('humidity', 0))
        temperature_celsius     = int(event.get('temperature_celsius', 0))
        temperature_fahrenheit  = int(event.get('temperature_fahrenheit', 0))
        msg_count               = int(event.get('msg_count', 0))
        uptime_ms               = int(event.get('uptime_ms', 0))
        timestamp               = int(event.get('timestamp', time.time() * 1000))

        item = {
            'device_id'   : device_id,
            'timestamp'   : timestamp,
            'humidity'    : humidity,
            'temperature_celsius' : temperature_celsius,
            'temperature_fahrenheit' : temperature_fahrenheit,
            'msg_count'   : msg_count,
            'uptime_ms'   : uptime_ms
        }

        table.put_item(Item=item)
        print(f"Saved: {humidity} | temperature: {temperature_celsius}°C | device: {device_id}")

        return {
            'statusCode': 200,
            'body': json.dumps(f'{humidity} - {temperature_celsius}°C saved successfully')
        }

    except Exception as e:
        print(f"Error: {str(e)}")
        return {
            'statusCode': 500,
            'body': json.dumps(f'Error: {str(e)}')
        }