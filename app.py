from flask import Flask, request, render_template, redirect
import sys
import json

import pandas as pd
import plotly
import plotly.express as px
 
app = Flask(__name__)

status = {
    "exh_stat":0,
    "hum_stat": 0,
    "aux1_stat": 0,
    "aux2_stat": 0,
    "light_stat": 0,
    "pump_stat": 0,
    "temp_rec": 0,
    "soil_rec": 0,
    "co2_rec": 0,
    
    # there are two available cycles represented by ints: 18h/6h is 1; 12h/12h is 2; 
    # You can add more and then change the code in index.html and the arduino script.
    "lightCyc": 1,
    "pumpCyc": 20,
    "aux1Cyc": 1,
    "aux2Cyc": 1,
    }
 
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
            status['pump_stat'] = int(request.form.get('pump_stat'))
            status['temp_rec'] = int(request.form.get('temp_rec'))
            status['soil_rec'] = int(request.form.get('soil_rec'))
            status['co2_rec'] = int(request.form.get('co2_rec'))

        elif request.form["form_id"] == "cycles":
            status['lightCyc'] = int(request.form.get('lightCyc'))
            status['pumpCyc'] = int(request.form.get('pumpCyc'))
            status['aux1Cyc'] = int(request.form.get('aux1Cyc'))
            status['aux2Cyc'] = int(request.form.get('aux2Cyc'))
            
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
    return json.dumps(status)


if __name__ == '__main__':
    # run app in debug mode on port 5000
    app.run(debug=True, port=5000, host="0.0.0.0")
    
    