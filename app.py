import errno
from flask import Flask, request, render_template, redirect
import sys
import json

import pandas as pd
import plotly
import plotly.express as px

import sqlite3 as sq
import os

import status_setup
 
app = Flask(__name__)

# there are two available cycles represented by ints: 18h/6h is 18; 12h/12h is 12; 
# the arduino code is made so that the number represents the duration of the first part of the cycle
# as such, if the value is 4, you would have 4h/20h cycle.
# You can add more and then change the code in index.html and the arduino script.
status = status_setup.status

#DATABASE CHECKS
#checks if database folder exists
folder_name = "sensor_database"
if not os.path.exists(folder_name):
    os.mkdir(folder_name)
    print(f"Created folder '{folder_name}'.")

#creates a database if one doesn't exist
db_path = f'{folder_name}/SensorData.db'
con = None
try:
    con = sq.connect(db_path)
    print("Database exists or was created.")
except sq.Error as e:
    print(errno)
finally:
    if con:
        con.close()

#verifies that the tables are correctly made
con = sq.connect(db_path)
cur = con.cursor() 

try:
    cur.execute("SELECT * FROM sensor_data")
    # storing the data in a list
    data_list = cur.fetchall() 
except:
    cur.execute("CREATE TABLE sensor_data(timestamp DATETIME, DHT_temp NUMERIC, DHT_hum NUMERIC, SOIL_hum NUMERIC, MQ135_ppm NUMERIC)")
    print("Created sensor table.")

con.commit()
con.close()
    
 
@app.route("/", methods=['GET', 'POST'])
def index():
    # handle the POST request
    if request.method == 'POST':
        print(request.form, file=sys.stdout)
        if request.form["form_id"] == "hardware":
            status['exh_stat'] = int(request.form.get('exh_stat'))
            status['aux1_stat'] = int(request.form.get('aux1_stat'))
            status['aux2_stat'] = int(request.form.get('aux2_stat'))
            status['light_stat'] = int(request.form.get('light_stat'))
            status['hum_stat'] = int(request.form.get('hum_stat'))
            # status['pump_stat'] = int(request.form.get('pump_stat'))
            status['temp_rec'] = int(request.form.get('temp_rec'))
            status['soil_rec'] = int(request.form.get('soil_rec'))
            status['co2_rec'] = int(request.form.get('co2_rec'))

        elif request.form["form_id"] == "cycles":
            status['lightCyc'] = int(request.form.get('lightCyc'))
            status['pumpCyc'] = int(request.form.get('pumpCyc'))
            # status['aux1Cyc'] = int(request.form.get('aux1Cyc'))
            # status['aux2Cyc'] = int(request.form.get('aux2Cyc'))

        f = open("status_setup.py","w")
        f.write("status = " + str(status))
        f.close()
        print("Successfully updated status file.")
            
        return redirect('/#navigbar')
    
    df = pd.DataFrame({
        "Fruit": ["Apples", "Oranges", "Bananas", "Apples", "Oranges", "Bananas"],
        "Amount": [4, 1, 2, 2, 4, 5],
        "City": ["SF", "SF", "SF", "Montreal", "Montreal", "Montreal"]
    })

    fig1 = px.bar(df, x="Fruit", y="Amount", color="City", barmode="group")
    graphJSON1 = json.dumps(fig1, cls=plotly.utils.PlotlyJSONEncoder)
    header1="Fruit in North America"
    description1 = """
    A academic study of the number of apples, oranges and bananas in the cities of
    San Francisco and Montreal would probably not come up with this chart.
    """

    return render_template("index.html", status=status, graphJSON1=graphJSON1, description1=description1)

@app.route('/get_data')
def get_data():
    add_data(55, 21, 35, 1134)
    return json.dumps(status)

@app.route('/sensor_data', methods = ['POST'])
def postJsonHandler():
    content = request.get_json()
    print (content)
    return 'JSON posted'


def add_data(DHT_hum, DHT_temp, SOIL_hum, MQ135_ppm):
    con = sq.connect(db_path)
    curs = con.cursor()

    curs.execute("INSERT INTO sensor_data values(datetime('now'), (?),(?),(?),(?))", (DHT_temp, DHT_hum,SOIL_hum,MQ135_ppm))

    con.commit()
    con.close()
    


if __name__ == '__main__':
    # run app in debug mode on port 5000
    app.run(port=5000, host="0.0.0.0")
    
    