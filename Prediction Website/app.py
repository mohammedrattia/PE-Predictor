from flask import Flask, render_template, request, jsonify
import firebase_admin
from firebase_admin import credentials, db
import joblib
import numpy as np

app = Flask(__name__)

# Firebase configuration
cred = credentials.Certificate("D:/_STUDY_/Capstone/Model/creds.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://dvt-predictor-default-rtdb.europe-west1.firebasedatabase.app/'
})

# Load the Random Forest model
model_filename = 'D:/_STUDY_/Capstone/Model/random_forest_model.joblib'
loaded_rf_model = joblib.load(model_filename)

@app.route('/')
def home():
    return render_template('index.html')

# Endpoint for making predictions
@app.route('/predict', methods=['POST'])
def predict():
    try:
        # Get data from Firebase
        data_from_firebase = db.reference('/').get()
        
        # Extract values from the JSON data
        age = data_from_firebase['row']['Age']
        heart_rate = data_from_firebase['row']['Heart Rate']
        oxygen_saturation = data_from_firebase['row']['Oxygen Saturation']
        temperature = data_from_firebase['row']['Temperature']

        # Create a list with the extracted values
        input_data = [age, heart_rate, oxygen_saturation, temperature]

        # Make predictions with the loaded model
        input_data_array = np.array([input_data])  # Assuming the model expects a 2D array
        prediction = loaded_rf_model.predict(input_data_array)

        # Render the prediction on the HTML template
        if prediction[0]:
            return render_template('index.html', prediction_text='You have high probability of PE complications.')
        return render_template('index.html', prediction_text='You have low probability of PE complications.')

    except Exception as e:
        return render_template('index.html', prediction_text=f'Error: {str(e)}')

if __name__ == '__main__':
    app.run(debug=True)
