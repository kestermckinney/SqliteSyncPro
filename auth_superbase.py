import requests

SUPABASE_URL = "https://nrtjpzkrldwydkbopsml.supabase.co"
API_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im5ydGpwemtybGR3eWRrYm9wc21sIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzM4NTU0NTQsImV4cCI6MjA4OTQzMTQ1NH0.hzzyb5bFKDIFbrJ7Fa8INh57pWIkz52csQ2gQ_L302E"

EMAIL = "paul.mckinney@gmail.com"
PASSWORD = "testuser1"

# 🔐 Step 1: Log in to get JWT
auth_url = f"{SUPABASE_URL}/auth/v1/token?grant_type=password"

auth_response = requests.post(
    auth_url,
    headers={
        "apikey": API_KEY,
        "Content-Type": "application/json"
    },
    json={
        "email": EMAIL,
        "password": PASSWORD
    }
)

auth_data = auth_response.json()

if "access_token" not in auth_data:
    raise Exception(f"Login failed: {auth_data}")

access_token = auth_data["access_token"]

print("✅ Got JWT:", access_token[:30] + "...")

# 📡 Step 2: Use JWT to fetch records
rest_url = f"{SUPABASE_URL}/rest/v1/todos"

data_response = requests.get(
    rest_url,
    headers={
        "apikey": API_KEY,
        "Authorization": f"Bearer {access_token}"
    }
)

data = data_response.json()

print("📦 Data from Supabase:")
for row in data:
    print(row)

