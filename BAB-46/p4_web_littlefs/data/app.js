function el(id) { return document.getElementById(id); }

function aturKoneksi(ok) {
  el("koneksi").textContent = ok ? "● Terhubung" : "● Terputus";
  el("koneksi").className = "koneksi " + (ok ? "ok" : "putus");
}

async function perbarui() {
  try {
    const r = await fetch("/api/sensor", { cache: "no-store" });
    if (!r.ok) throw new Error("HTTP " + r.status);
    const d = await r.json();
    el("suhu").textContent = d.suhu === null ? "--" : d.suhu.toFixed(1).replace(".", ",");
    el("lembap").textContent = d.kelembapan === null ? "--" : d.kelembapan.toFixed(0);
    el("pot").textContent = d.potensiometer;
    el("jam").textContent = new Date().toLocaleTimeString("id-ID");
    aturKoneksi(true);
  } catch (e) {
    aturKoneksi(false);
  }
  setTimeout(perbarui, 2000);
}

perbarui();
