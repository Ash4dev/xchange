import streamlit as st
import pandas as pd
from datetime import datetime
import re

# --- Constants ---

ORDER_TYPES = [
    "AllOrNone",
    "GoodTillCancel",
    "GoodTillDate",
    "GoodForDay",
    "GoodAfterTime",
    "MarketOnOpen",
    "MarketOnClose",
    "ImmediateOrCancel",
    "FillOrKill",
    "Market",
    "-"
]

SIDES = ["Buy", "Sell", "-"]

INCOMING_COLUMNS = [
    "OrderCounter", "Action", "Symbol", "OrderType", "Side",
    "Price", "Quantity", "ActivateTime", "DeactivateTime", "ParticipantID"
]

# --- Validators ---

def validate_positive_integer(val, col_name, row_idx):
    if pd.isna(val) or str(val).strip() in ["-", ""]:
        return None
    if not str(val).isdigit() or int(val) <= 0:
        return f"Row {row_idx+1} in column '{col_name}' must be a positive integer or '-'."
    return None

def validate_datetime(val, col_name, row_idx):
    if pd.isna(val) or str(val).strip() in ["-", ""]:
        return None
    try:
        datetime.strptime(str(val), "%d-%m-%Y %H:%M:%S")
    except ValueError:
        return f"Row {row_idx+1} in column '{col_name}' must be in format %d-%m-%Y %H:%M:%S or '-'."
    return None

def validate_participant_id(val, col_name, row_idx):
    if pd.isna(val) or str(val).strip() in ["-", ""]:
        return None
    val = str(val)
    m = re.fullmatch(r"(\d+)_([A-Za-z0-9]{4})", val)
    if not m:
        return f"Row {row_idx+1} in column '{col_name}' must be digits_part2 where part2 is exactly 4 alphanumeric characters, or '-'."
    return None

def validate_symbol(val, col_name, row_idx):
    if pd.isna(val) or str(val).strip() in ["-", ""]:
        return None
    val = str(val).strip()
    if len(val) != 3 or not val.isalpha():
        return f"Row {row_idx+1}: '{col_name}' must be exactly 3 letters (A-Z) or '-'."
    return None

def validate_order_counter(val, col_name, row_idx):
    if pd.isna(val) or str(val).strip() == "":
        return f"Row {row_idx+1} in column '{col_name}' is required."
    if str(val).strip() == "-":
        return f"Row {row_idx+1} in column '{col_name}' cannot be '-'."
    if not str(val).isdigit() or int(val) <= 0:
        return f"Row {row_idx+1} in column '{col_name}' must be a positive integer."
    return None

def validate_dataframe(df, validations, df_name):
    errors = []
    for idx, row in df.iterrows():
        for col, validator in validations.items():
            if col in df.columns:
                err = validator(row[col], col, idx)
                if err:
                    errors.append(f"[{df_name}] {err}")
    return errors

# --- Time Validation and Defaulting ---

DEFAULT_ACTIVATE_TIME = datetime.now().strftime("%d-%m-%Y/%H:%M:%S")
DEFAULT_DEACTIVATE_TIME = "01-01-2100/00:00:00"

def validate_and_fix_times(row, idx):
    errors = []

    order_type = row.get("OrderType", None)
    activate_time = row.get("ActivateTime", None)
    deactivate_time = row.get("DeactivateTime", None)

    if str(order_type).strip() in ["-", "", "nan"]:
        order_type = None

    # Check mandatory fields
    if order_type == "GoodAfterTime":
        if pd.isna(activate_time) or str(activate_time).strip() in ["-", ""]:
            errors.append(
                f"Row {idx+1}: 'ActivateTime' is required for OrderType 'GoodAfterTime'."
            )
    else:
        if pd.isna(activate_time) or str(activate_time).strip() in ["-", ""]:
            row["ActivateTime"] = DEFAULT_ACTIVATE_TIME

    if order_type in ("GoodTillCancel", "GoodForDay"):
        if pd.isna(deactivate_time) or str(deactivate_time).strip() in ["-", ""]:
            errors.append(
                f"Row {idx+1}: 'DeactivateTime' is required for OrderType '{order_type}'."
            )
    else:
        if pd.isna(deactivate_time) or str(deactivate_time).strip() in ["-", ""]:
            row["DeactivateTime"] = DEFAULT_DEACTIVATE_TIME

    # Validate formats
    time_vals = {}
    for field in ["ActivateTime", "DeactivateTime"]:
        val = row.get(field, None)
        if pd.isna(val) or str(val).strip() in ["-", ""]:
            continue
        try:
            time_vals[field] = datetime.strptime(str(val), "%d-%m-%Y/%H:%M:%S")
        except ValueError:
            errors.append(
                f"Row {idx+1}: '{field}' must be in format %d-%m-%Y/%H:%M:%S or '-'."
            )

    if "ActivateTime" in time_vals and "DeactivateTime" in time_vals:
        if time_vals["ActivateTime"] > time_vals["DeactivateTime"]:
            errors.append(
                f"Row {idx+1}: 'ActivateTime' cannot be after 'DeactivateTime'."
            )
    return errors, row

def validate_incoming_orders(df):
    errors = []
    updated_rows = []
    for idx, row in df.iterrows():
        row = row.copy()

        # Validate OrderCounter
        err = validate_order_counter(row.get("OrderCounter", None), "OrderCounter", idx)
        if err:
            errors.append(f"[Incoming Orders] {err}")

        # Validate Action
        action = str(row.get("Action", "-")).strip()
        if action not in ["Add", "Modify", "Cancel"]:
            errors.append(f"[Incoming Orders] Row {idx+1}: Invalid action '{action}'.")

        # Validate fields only if not Cancel
        if action != "Cancel":
            err = validate_symbol(row.get("Symbol", None), "Symbol", idx)
            if err:
                errors.append(f"[Incoming Orders] {err}")

            err_list, row = validate_and_fix_times(row, idx)
            errors.extend([f"[Incoming Orders] {e}" for e in err_list])

            for col in ["Price", "Quantity"]:
                err = validate_positive_integer(row.get(col, None), col, idx)
                if err:
                    errors.append(f"[Incoming Orders] {err}")

            err = validate_participant_id(row.get("ParticipantID", None), "ParticipantID", idx)
            if err:
                errors.append(f"[Incoming Orders] {err}")

        updated_rows.append(row)
    new_df = pd.DataFrame(updated_rows) if updated_rows else df
    return errors, new_df

# --- Streamlit UI ---

st.set_page_config(page_title="Orderbook Test File Generator", layout="wide")
st.title("Orderbook Test File Generator")

test_name = st.text_input("Test Case Name (without .txt):")

st.markdown("---")

col1, col2 = st.columns(2)
with col1:
    pending_order_threshold = st.number_input(
        "PendingOrderThreshold",
        min_value=0,
        value=3,
        step=1
    )
with col2:
    pending_duration_threshold = st.number_input(
        "PendingDurationThreshold",
        min_value=0,
        value=100,
        step=1
    )

# Incoming Orders

st.header("Incoming Orders")

column_config = {
    "OrderCounter": st.column_config.NumberColumn(
        "Order Counter",
        min_value=1,
        step=1
    ),
    "Action": st.column_config.SelectboxColumn(
        "Action",
        options=["Add", "Modify", "Cancel"],
        default="Add",
        required=True
    ),
    "OrderType": st.column_config.SelectboxColumn(
        "Order Type",
        options=ORDER_TYPES,
        required=False
    ),
    "Side": st.column_config.SelectboxColumn(
        "Side",
        options=SIDES,
        required=False
    )
}

# Initialize empty row with all '-'
empty_row = {col: "-" for col in INCOMING_COLUMNS}
df_orders = pd.DataFrame([empty_row])

incoming_orders = st.data_editor(
    df_orders,
    num_rows="dynamic",
    use_container_width=True,
    key="incoming_orders",
    column_config=column_config
)

# Preprocessor
st.header("Preprocessor Output")

col1, col2 = st.columns(2)
with col1:
    st.subheader("Bids")
    preprocessor_bids = st.data_editor(
        pd.DataFrame(columns=["Symbol", "Side", "Action", "Price", "Quantity", "ParticipantID"]),
        num_rows="dynamic",
        use_container_width=True,
        key="pre_bids",
        column_config=column_config
    )
with col2:
    st.subheader("Asks")
    preprocessor_asks = st.data_editor(
        pd.DataFrame(columns=["Symbol", "Side", "Action", "Price", "Quantity", "ParticipantID"]),
        num_rows="dynamic",
        use_container_width=True,
        key="pre_asks",
        column_config=column_config
    )

# Orderbook
st.header("Orderbook Output")

col1, col2 = st.columns(2)
with col1:
    st.subheader("Bids")
    orderbook_bids = st.data_editor(
        pd.DataFrame(columns=["Symbol", "Side", "Price", "Quantity", "OrderListSize"]),
        num_rows="dynamic",
        use_container_width=True,
        key="ob_bids",
        column_config=column_config
    )
with col2:
    st.subheader("Asks")
    orderbook_asks = st.data_editor(
        pd.DataFrame(columns=["Symbol", "Side", "Price", "Quantity", "OrderListSize"]),
        num_rows="dynamic",
        use_container_width=True,
        key="ob_asks",
        column_config=column_config
    )

# Trades
st.header("Trades")

trades_data = st.data_editor(
    pd.DataFrame(columns=["Symbol", "SettlementPrice", "Quantity", "BuyerID", "SellerID"]),
    num_rows="dynamic",
    use_container_width=True,
    key="trades"
)

st.markdown("---")

def format_table(df: pd.DataFrame) -> str:
    if df.empty:
        return "<empty>\n"
    col_widths = [max(len(str(col)), df[col].astype(str).map(len).max()) for col in df.columns]
    sep = "| " + " | ".join(["{:<" + str(w) + "}" for w in col_widths]) + " |"
    out = "-" * (sum(col_widths) + 3 * len(col_widths) + 1) + "\n"
    out += sep.format(*df.columns) + "\n"
    out += "-" * (sum(col_widths) + 3 * len(col_widths) + 1) + "\n"
    for _, row in df.iterrows():
        out += sep.format(*row) + "\n"
    out += "-" * (sum(col_widths) + 3 * len(col_widths) + 1) + "\n"
    return out

if st.button("Generate Test File"):
    errors = []

    # Validate incoming orders
    incoming_errors, incoming_orders_fixed = validate_incoming_orders(incoming_orders)
    errors.extend(incoming_errors)

    # Validate other DataFrames
    errors += validate_dataframe(
        preprocessor_bids,
        {
            "Symbol": validate_symbol,
            "Price": validate_positive_integer,
            "Quantity": validate_positive_integer,
            "ParticipantID": validate_participant_id,
        },
        "Preprocessor Bids"
    )
    errors += validate_dataframe(
        preprocessor_asks,
        {
            "Symbol": validate_symbol,
            "Price": validate_positive_integer,
            "Quantity": validate_positive_integer,
            "ParticipantID": validate_participant_id,
        },
        "Preprocessor Asks"
    )
    errors += validate_dataframe(
        orderbook_bids,
        {
            "Symbol": validate_symbol,
            "Price": validate_positive_integer,
            "Quantity": validate_positive_integer,
            "OrderListSize": validate_positive_integer,
        },
        "Orderbook Bids"
    )
    errors += validate_dataframe(
        orderbook_asks,
        {
            "Symbol": validate_symbol,
            "Price": validate_positive_integer,
            "Quantity": validate_positive_integer,
            "OrderListSize": validate_positive_integer,
        },
        "Orderbook Asks"
    )
    errors += validate_dataframe(
        trades_data,
        {
            "Symbol": validate_symbol,
            "SettlementPrice": validate_positive_integer,
            "Quantity": validate_positive_integer,
            "BuyerID": validate_participant_id,
            "SellerID": validate_participant_id,
        },
        "Trades"
    )

    if errors:
        for e in errors:
            st.error(e)
    else:
        filename = f"{test_name.strip()}.txt"
        with open(filename, "w") as f:
            f.write("---------------------ARGUMENTS---------------------\n")
            f.write(f"PendingOrderThreshold: {pending_order_threshold}\n")
            f.write(f"PendingDurationThreshold: {pending_duration_threshold}\n")
            f.write("---------------------INCOMING ORDERS---------------\n")
            if not incoming_orders_fixed.empty:
                f.write(format_table(incoming_orders_fixed))
            f.write("---------------------EXPECTED RESULT---------------\n")
            f.write("---------------------PREPROCESSOR------------------\n")
            if not preprocessor_bids.empty:
                f.write(format_table(preprocessor_bids))
            if not preprocessor_asks.empty:
                f.write(format_table(preprocessor_asks))
            f.write("---------------------PREPROCESSOR END--------------\n")
            f.write("---------------------ORDERBOOK---------------------\n")
            if not orderbook_bids.empty:
                f.write(format_table(orderbook_bids))
            if not orderbook_asks.empty:
                f.write(format_table(orderbook_asks))
            f.write("---------------------ORDERBOOK END-----------------\n")
            f.write("---------------------TRADES------------------------\n")
            if not trades_data.empty:
                f.write(format_table(trades_data))
            f.write("---------------------TRADES END--------------------\n")

        st.success(f"Test file '{filename}' generated successfully.")

