(function () {
  var status = document.getElementById("network-status");
  if (!status) {
    return;
  }

  function paint() {
    var online = navigator.onLine;
    status.textContent = online ? "online" : "offline";
    status.setAttribute("data-state", online ? "online" : "offline");
  }

  paint();
  window.addEventListener("online", paint);
  window.addEventListener("offline", paint);

  var retry = document.getElementById("retry-btn");
  if (retry) {
    retry.addEventListener("click", function () {
      location.reload();
    });
  }
})();
