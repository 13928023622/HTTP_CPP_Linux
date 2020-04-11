from flask import Flask, jsonify, request
import base64
import random
import re
from io import BytesIO
from PIL import Image
import os

app = Flask(__name__)
IMAGE_STORE_PATH = "resources/image/upload"
if not os.path.exists(IMAGE_STORE_PATH):
    os.makedirs(IMAGE_STORE_PATH)

def get_uuid(length=32, radix=16):
    '''
        Generate UUID 
        @param len : length of uuid
        @param radix : radix of number(binary, octal, decimal, hex)
        @returns uuid
        EX. 
            get_uuid(32, 2)  --> 10000110111110000011111001001001
            get_uuid(32, 8)  --> 32533630162357503061604046713330
            get_uuid(32, 10) --> 81331477828088994361772282974201
            get_uuid(32, 16) --> 710F7ADAB1132A0FDD2C05EFD64F2684
            get_uuid(0, 36)  --> A552CFD0-CCFA-48AF-1082E-0F66EBA215A9
    '''
    chars = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
    uuid = []
    radix = radix or length(chars)
    print("radix: ", radix)
    if length > 0:
        # Compact form
        for _ in range(length):
            n = 0 | int(random.random()*radix)
            # print("i:", i)
            uuid.append(chars[n])
    else:
        for i in range(36):
            uuid.append('')
        # rfc4122, version 4 form
        # rfc4122 requires these characters
        uuid[8] = uuid[13] = uuid[18] = uuid[23] = '-'
        uuid[14] = '4'

        # Fill in random data.  At i==19 set the high bits of clock sequence as
        # per rfc4122, sec. 4.1.5
        for i in range(36):
            if (not uuid[i]):
                r = 0 | int(random.random()*16)
                uuid[i] = str(((r & 0x3) | 0x8)) if i == 19 else chars[r]
    return "".join(uuid)
 
 
def base64_to_image(base64_str, format='jpg'):
    '''
    Function Name： base64_to_image
    Parameters：
        base64_str: base64 string
    Return Value：
        Image stored path
    What it can do：
        convert base64 to image and save it.
    '''
    base64_data = re.sub('^data:image/.+;base64,', '', base64_str)
    try:
        byte_data = base64.b64decode(base64_data, validate=True)
        image_data = BytesIO(byte_data)
        img = Image.open(image_data)

        # Check format is validation
        # Todo

        if format == 'jpg':
            file_name = os.path.join(IMAGE_STORE_PATH, get_uuid()+"."+format)
            img.save(file_name, quality=95)
            return file_name
    except base64.binascii.Error as bError:
        print(bError)
        return None
    except IOError as e:
        print(e)
        return None


@app.route('/test', methods=['GET','POST'])
def test():
    return 'Hello, This is server'


@app.route('/upload_image', methods=['POST'])
def upload_image():
    return_json = {}

    if request.method != 'POST':
        return_json['status_code'] = 1001
        return_json['message'] = 'Request method is Wrong.'
        return jsonify(return_json)
    # receive data
    print('request.content_length:', request.content_length)
    print('request.accept_mimetypes:', request.accept_mimetypes)
    print('request.user_agent:', request.user_agent)
    # print('request.json: ', request.json)
    # print('request.data: ', request.data)

    if request.json is None:
        return_json['status_code'] = 1002
        return_json['message'] = 'Json data is None.'
        return jsonify(return_json)
    if 'img_base64' not in request.json.keys():
        return_json['status_code'] = 1003
        return_json['message'] = 'Argument \'img_base64\' of Json date does not exists.'
        return jsonify(return_json)
    # Get json data from request
    path = base64_to_image(request.json['img_base64'])
    if path is not None:
        return_json['status_code'] = 200
        return_json['image_path'] = '/'+path
        return jsonify(return_json)
    else:
        return jsonify({'status_code':1004, 'message': 'Invalid base64 string'})


if __name__ == '__main__':
    app.run('0.0.0.0', 8520, use_reloader=True)

    # get_uuid() test
    # print(get_uuid(32, 2))
    # print(get_uuid(32, 8))
    # print(get_uuid(32, 10))
    # print(get_uuid(32, 16))
    # print(get_uuid(0, 16))
