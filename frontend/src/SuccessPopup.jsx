import React from "react";
import "./SuccessPopup.css";

const SuccessPopup = ({ message, onClose }) => {
  return (
    <div className="popup" role="alert">
      <div className="popup-content">
        <span className="popup-message">{message}</span>
        <button onClick={onClose} className="popup-close-btn" aria-label="Luk popup">
          Luk
        </button>
      </div>
    </div>
  );
};

export default SuccessPopup;
