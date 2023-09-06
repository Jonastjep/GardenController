import errno
from flask import Flask, request, render_template, redirect
import sys
import json
from pprint import pprint

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

serv_key = "C4Xw4LhlIu"

#the number of datapoints shown on the graphs
form_vals = {"graph_data_pts": 20}

@app.route("/", methods=['GET', 'POST'])
def index():
    # handle the POST request
    if request.method == 'POST':
        
        if request.form["form_id"] == "hardware":
            status['exh_stat'] = int(request.form.get('exh_stat'))
            status['aux1_stat'] = int(request.form.get('aux1_stat'))
            status['aux2_stat'] = int(request.form.get('aux2_stat'))
            status['light_stat'] = int(request.form.get('light_stat'))
            status['hum_stat'] = int(request.form.get('hum_stat'))
            status['pump_stat'] = int(request.form.get('pump_stat'))
            status['temp_rec'] = int(request.form.get('temp_rec'))
            status['soil_rec'] = int(request.form.get('soil_rec'))
            status['co2_rec'] = int(request.form.get('co2_rec'))

        elif request.form["form_id"] == "cycles":
            status['lightCyc'] = int(request.form.get('lightCyc'))
            status['pumpCyc'] = int(request.form.get('pumpCyc'))
            # status['aux1Cyc'] = int(request.form.get('aux1Cyc'))
            # status['aux2Cyc'] = int(request.form.get('aux2Cyc'))

        elif request.form["form_id"] == "graphs":
            val = request.form.get('nb_datapts')
            print(val)
            if val.isdigit():
                form_vals['graph_data_pts'] = int(val)


        #UPDATE THE STATUS FILE TO KEEP CURRENT SETTINGS
        f = open("status_setup.py","w")
        f.write("status = ")
        pprint(status, f, sort_dicts=False)
        f.close()
        print("Successfully updated status file.")

        if request.form["form_id"] == "graphs":
            return redirect('/#hum')
        else:
            return redirect('/#navigbar')
    
    cnx = sq.connect(db_path)
    df = pd.read_sql_query(f"SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT {form_vals['graph_data_pts']}", cnx)

    print(df)

    fig_hum = px.line(df, x="timestamp", y=["DHT_hum","SOIL_hum"])
    graphJSON_hum = json.dumps(fig_hum, cls=plotly.utils.PlotlyJSONEncoder)
    description_hum = """Plot of air and soil humidity in the grow chamber/pot."""

    fig_temp = px.line(df, x="timestamp", y="DHT_temp")
    graphJSON_temp = json.dumps(fig_temp, cls=plotly.utils.PlotlyJSONEncoder)
    description_temp = """Plot of air temperature in the grow chamber."""

    fig_ppm = px.line(df, x="timestamp", y="MQ135_ppm")
    graphJSON_ppm = json.dumps(fig_ppm, cls=plotly.utils.PlotlyJSONEncoder)
    description_ppm = """Plot of the CO2 ppm in the grow chamber."""

    fig_all = px.line(df, x="timestamp", y=["DHT_hum","SOIL_hum","DHT_temp","MQ135_ppm"])
    graphJSON_all = json.dumps(fig_all, cls=plotly.utils.PlotlyJSONEncoder)
    description_all = """Plot of all stored sensor data (seen in the other plots) together."""

    plot_dict = {'hum':{'data':graphJSON_hum, 'descr': description_hum},
                 'temp':{'data':graphJSON_temp, 'descr': description_temp},
                 'ppm':{'data':graphJSON_ppm, 'descr': description_ppm},
                 'all':{'data':graphJSON_all, 'descr': description_all},
                 }
    
    return render_template("index.html", 
                           status=status, 
                           plot_dict=plot_dict,
                           nb_datapts=form_vals['graph_data_pts'], 
                           )

@app.route('/get_data')
def get_data():
    return json.dumps(status)

@app.route('/sensor_data', methods = ['POST'])
def postJsonHandler():
    content = request.get_json()
    print(content)

    if(content['key'] == serv_key):
        add_data(content['sensor_data']["DHT_hum"], content['sensor_data']["DHT_temp"], content['sensor_data']["SOIL_hum"], content['sensor_data']["MQ135"]) #log sensor data into the database
        return 'Data posted'
    else:
        print("Wrong server key. Cannot log data.")
        return 'Wrong key.'
    


def add_data(DHT_hum, DHT_temp, SOIL_hum, MQ135_ppm):
    con = sq.connect(db_path)
    curs = con.cursor()

    curs.execute("INSERT INTO sensor_data values(datetime('now'), (?),(?),(?),(?))", (DHT_temp, DHT_hum,SOIL_hum,MQ135_ppm))

    con.commit()
    con.close()
    


if __name__ == '__main__':
    # run app in debug mode on port 5000
    app.run(port=5000, host="0.0.0.0", debug=True)
    
    