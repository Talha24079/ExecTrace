#!/bin/bash
# Multi-Project Isolation Test Script

echo "🧪 Testing Multi-Project Isolation..."
echo "========================================"

# Register 2 users
echo -e "\n1. Registering two users..."
USER1_RESP=$(curl -s -X POST http://localhost:8080/register -d "username=alice")
USER2_RESP=$(curl -s -X POST http://localhost:8080/register -d "username=bob")

USER1_KEY=$(echo "$USER1_RESP" | grep -o '"api_key":[[:space:]]*"[^"]*' | sed 's/.*"\([^"]*\)$/\1/')
USER2_KEY=$(echo "$USER2_RESP" | grep -o '"api_key":[[:space:]]*"[^"]*' | sed 's/.*"\([^"]*\)$/\1/')

echo "   Alice user key: $USER1_KEY"
echo "   Bob user key: $USER2_KEY"

# Create projects for each user
echo -e "\n2. Creating projects for each user..."
PROJ1_RESP=$(curl -s -X POST "http://localhost:8080/projects/create?name=AliceApp" -H "X-User-Key: $USER1_KEY")
PROJ2_RESP=$(curl -s -X POST "http://localhost:8080/projects/create?name=BobApp" -H "X-User-Key: $USER2_KEY")

PROJ1_KEY=$(echo "$PROJ1_RESP" | grep -o '"project_key":[[:space:]]*"[^"]*' | sed 's/.*"\([^"]*\)$/\1/')
PROJ2_KEY=$(echo "$PROJ2_RESP" | grep -o '"project_key":[[:space:]]*"[^"]*' | sed 's/.*"\([^"]*\)$/\1/')

echo "   Alice project key: $PROJ1_KEY"
echo "   Bob project key: $PROJ2_KEY"

# Log to each project
echo -e "\n3. Logging events to each project..."
curl -s -X POST http://localhost:8080/log -H "X-API-Key: $PROJ1_KEY" \
  -d "func=alice_function&msg=Alice_testing&duration=100&ram=50" > /dev/null
curl -s -X POST http://localhost:8080/log -H "X-API-Key: $PROJ2_KEY" \
  -d "func=bob_function&msg=Bob_testing&duration=200&ram=75" > /dev/null

echo "   ✓ Logged to AliceApp"
echo "   ✓ Logged to BobApp"

# Query each project - should see only own logs
echo -e "\n4. Querying logs for each project..."
ALICE_LOGS=$(curl -s http://localhost:8080/query -H "X-API-Key: $PROJ1_KEY")
BOB_LOGS=$(curl -s http://localhost:8080/query -H "X-API-Key: $PROJ2_KEY")

echo -e "\n   Alice's logs:"
echo "   $ALICE_LOGS" | jq '.' 2>/dev/null || echo "   $ALICE_LOGS"

echo -e "\n   Bob's logs:"
echo "   $BOB_LOGS" | jq '.' 2>/dev/null || echo "   $BOB_LOGS"

# Verify isolation
echo -e "\n5. Verifying isolation..."
ALICE_HAS_ALICE=$(echo "$ALICE_LOGS" | grep -c "alice_function")
ALICE_HAS_BOB=$(echo "$ALICE_LOGS" | grep -c "bob_function")
BOB_HAS_BOB=$(echo "$BOB_LOGS" | grep -c "bob_function")
BOB_HAS_ALICE=$(echo "$BOB_LOGS" | grep -c "alice_function")

if [ "$ALICE_HAS_ALICE" -gt 0 ] && [ "$ALICE_HAS_BOB" -eq 0 ] && \
   [ "$BOB_HAS_BOB" -gt 0 ] && [ "$BOB_HAS_ALICE" -eq 0 ]; then
    echo "   ✅ ISOLATION TEST PASSED"
    echo "   - Alice sees only her own logs ✓"
    echo "   - Bob sees only his own logs ✓"
    exit 0
else
    echo "   ❌ ISOLATION TEST FAILED"
    echo "   - Alice has alice_function: $ALICE_HAS_ALICE (expected > 0)"
    echo "   - Alice has bob_function: $ALICE_HAS_BOB (expected 0)"
    echo "   - Bob has bob_function: $BOB_HAS_BOB (expected > 0)"
    echo "   - Bob has alice_function: $BOB_HAS_ALICE (expected 0)"
    exit 1
fi
