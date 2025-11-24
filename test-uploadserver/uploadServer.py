from flask import Flask, request
import os

app = Flask(__name__)
UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route("/upload", methods=["POST"])
def upload_file():
    if "fileUpload" not in request.files:
        return "No file field", 400

    uploaded_file = request.files["fileUpload"]
    save_path = os.path.join(UPLOAD_FOLDER, uploaded_file.filename)
    uploaded_file.save(save_path)

    return f"File received: {uploaded_file.filename}\n", 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=27015, ssl_context=("cert.pem", "key.pem"))